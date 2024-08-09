//////////////////////////////////////////////////////////////////////////////
// This file is part of 'bldDecode'.
// It is subject to the license terms in the LICENSE.txt file found in the 
// top-level directory of this distribution and at: 
//    https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html. 
// No part of 'bldDecode', including this file, 
// may be copied, modified, propagated, or distributed except according to 
// the terms contained in the LICENSE.txt file.
//////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <getopt.h>
#include <limits.h>
#include <stdint.h>
#include <signal.h>
#include <vector>
#include <algorithm>
#include <cassert>
#include <stdarg.h>

#include <epicsTime.h>

#include "pvxs/client.h"
#include "pvxs/data.h"

#include "util.h"
#include "report.h"
#include "bld-proto.h"

#define MAXLINE 9000

using ChannelType = pvxs::TypeCode::code_t;

static void cleanup();

static void print_data(uint32_t* data, size_t num, const std::vector<ChannelType>& formats, const std::vector<int>& channels, uint64_t);
static void usage(const char* argv0);
static std::vector<ChannelType> parse_channel_formats(const char* str);
static std::vector<int> parse_channels(const char* str);
static std::vector<int> parse_events(const char* str);
static std::vector<ChannelType> read_channel_formats(const char* str);
static void build_channel_list();
static void bld_printf(const char* fmt, ...) EPICS_PRINTF_STYLE(1,2);

static void timeoutHandler(int) {
    printf("Timeout exceeded, exiting!\n");
    exit(1);
}

static int show_data = 0;
static int unicast = 0;
static int verbose = 0;
static int quiet = 0;
static int generate_report = 0;
static std::vector<int> enabled_channels;
static std::vector<ChannelType> channel_formats;
static std::vector<int> events;
static int channel_remap[NUM_BLD_CHANNELS];     // Maps payload channels to their actual channel number
static Report* report;
static char reportFile[256] = "report.json";
static int num_channels = 0;

// List of channel labels
static std::vector<std::string> channel_labels = []() -> std::vector<std::string> {
    std::vector<std::string> labels;
    for (int i = 0; i < NUM_BLD_CHANNELS; ++i) {
        char ch[128];
        snprintf(ch, sizeof(ch), "ch%02d", i);
        labels.push_back(ch);
    }
    return labels;
}();

#define LOG_VERBOSE(...) if (verbose) { printf(__VA_ARGS__); }

static option long_opts[] = {
    {"port", required_argument, NULL, 'p'},
    {"show-data", no_argument, &show_data, 'd'},
    {"version", required_argument, NULL, 'k'},
    {"severity", required_argument, NULL, 's'},
    {"timeout", required_argument, NULL, 't'},
    {"num", required_argument, NULL, 'n'},
    {"format", required_argument, NULL, 'f'},
    {"unicast", no_argument, &unicast, 'u'},
    {"channels", required_argument, NULL, 'c'},
    {"help", no_argument, NULL, 'h'},
    {"events", required_argument, NULL, 'e'},
    {"pv", required_argument, NULL, 'b'},
    {"verbose", no_argument, &verbose, 'v'},
    {"address", required_argument, NULL, 'a'},
    {"report", no_argument, NULL, 'r'},
    {"output", required_argument, NULL, 'o'},
    {"quiet", no_argument, NULL, 'q'},
};

static const char* help_text[] = {
    "The port to use (default: 50000)",
    "Display event data",
    "Filter packets by this version",
    "Filter packets by this severity mask",
    "Timeout to receive packets, in seconds",
    "Number of packets to receive before exiting",
    "Data format (i.e. 'f,u,i,f' for float, uint32, int32, float)",
    "Receive packets as unicast too",
    "Channels to display (i.e. '1,2,5' will display channels 1, 2 and 5)",
    "Display this help text",
    "Event indices to display (i.e. '0,1,3' will display events 0, 1 and 3)",
    "PV that contains a description of the BLD payload",
    "Run in verbose mode, showing additional debugging info",
    "Multicast address",
    "Run in report generation mode",
    "File to place the generated report",
    "Disable all non-critical logging",
};

STATIC_ASSERT(arrayLength(long_opts) == arrayLength(help_text));

int main(int argc, char *argv[]) {
    int sockfd;
    char buffer[MAXLINE];

    char mcastAddr[256] = "224.0.0.0";

    int port = DEFAULT_BLD_PORT, needsSevr = 0;
    int64_t version = -1, numPackets = INT64_MAX;
    uint64_t timeout = UINT64_MAX;
    uint64_t sevrMask = 0;

    for (size_t i = 0; i < arrayLength(channel_remap); ++i)
        channel_remap[i] = i;

    signal(SIGALRM, timeoutHandler);
    signal(SIGINT, [](int) {cleanup(); exit(0);});

    int opt = 0, longind = 0;
    while ((opt = getopt_long(argc, argv, "rqvuhda:p:k:s:t:n:f:c:e:b:o:", long_opts, &longind)) != -1) {
        /* Handle long opts */
        if (opt == 0) {
            if (long_opts[longind].val != 0)
                opt = long_opts[longind].val;
        }

        switch(opt) {
        case 0:
            break;
        case 'v':
            verbose = 1;
            break;
        case 'd':
            show_data = 1;
            break;
        case 'p':
            port = atoi(optarg);
            break;
        case 'k':
            version = atoll(optarg);
            break;
        case 's':
            sevrMask = strtoull(optarg, NULL, num_str_base(optarg));
            needsSevr = 1;
            break;
        case 't':
            timeout = strtoull(optarg, NULL, num_str_base(optarg));
            break;
        case 'n':
            numPackets = strtoull(optarg, NULL, num_str_base(optarg));
            break;
        case 'h':
            usage(argv[0]);
            exit(0);
        case 'f':
            channel_formats = parse_channel_formats(optarg);
            break;
        case 'c':
            enabled_channels = parse_channels(optarg);
            break;
        case 'e':
            events = parse_events(optarg);
            break;
        case 'b':
            channel_formats = read_channel_formats(optarg);
            break;
        case 'u':
            unicast = 1;
            break;
        case 'a':
            strcpy_safe(mcastAddr, optarg);
            break;
        case 'o':
            strcpy_safe(reportFile, optarg);
            break;
        case 'r':
            generate_report = 1;
            report = new Report();
            break;
        case 'q':
            quiet = 1;
            break;
        case '?':
            usage(argv[0]);
            exit(EXIT_FAILURE);
            break;
        }
    }

    if (!enabled_channels.empty())
        build_channel_list();

    if (timeout != UINT_MAX)
        alarm(timeout);

    struct sockaddr_in servaddr, cliaddr;
    bldMulticastPacket_t *ptr;
    bldMulticastComplementaryPacket_t *compptr;

    // Creating socket file descriptor
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));

    // Filling server information
    servaddr.sin_family = AF_INET; // IPv4
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(port);

    // Bind the socket with the server address
    if ( bind(sockfd, (const struct sockaddr *)&servaddr,
            sizeof(servaddr)) < 0 )
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // If we're multicast, add ourselves to the group
    if (!unicast) {
        printf("Listening for multicast packets on %s\n", mcastAddr);
        ip_mreq mreq;
        mreq.imr_interface.s_addr = INADDR_ANY;
        mreq.imr_multiaddr.s_addr = inet_addr(mcastAddr);

        if (setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
            perror("failed to opt into multicast: setsockoptfailed");
            exit(EXIT_FAILURE);
        }
    }

    const bool ignoreFirst = !events.empty() && std::find(events.begin(), events.end(), 0) == events.end();

    const bool showData = show_data && !quiet && !report;

    PacketValidator validator;

    while(1)
    {
        // Clear buffer so we can easily cast to our structure types without printing junk
        memset(buffer, 0, sizeof(buffer));

        if (numPackets-- <= 0)
            break;

        char * bufptr = buffer;
        socklen_t len = sizeof(cliaddr); //len is value/result

        const ssize_t totalRead = recvfrom(sockfd, (char *)buffer, MAXLINE,
                                MSG_WAITALL, ( struct sockaddr *) &cliaddr,
                                &len);
        auto n = totalRead;
        if (n < 0) {
            perror("recvfrom failed");
            exit(EXIT_FAILURE);
        }

        size_t packSize = size_t(n) < sizeof(bldMulticastPacket_t) ? n : sizeof(bldMulticastPacket_t);
        ptr = (bldMulticastPacket_t *)buffer;

        // Check if we need to skip this packet
        if (version >= 0 && ptr->version != version)
            continue;

        // Now check if severity mask matches
        if (needsSevr && ptr->severityMask != sevrMask)
            continue;

        // Packet accepted for display, cancel any pending timeouts
        alarm(0);

        bld_printf("====== new packet size %li ======\n", n);

        LOG_VERBOSE("Received size: %li\n", n);

        const size_t payloadSize = sizeof(uint32_t) * num_channels;

        PacketError packetError;
        if ((packetError = validator.validate(ptr, packSize)) != PacketError::None) {
            printf("Invalid packet received: %s, len=%lu\n", to_string(packetError).c_str(), packSize);
            if (report)
                report->report_packet_error(packetError, buffer, n);
            continue;
        }

        if (!ignoreFirst) {
            uint32_t sec, nsec;
            extract_ts(ptr->timeStamp, sec, nsec);

            time_t sect = sec;
            auto tinfo = localtime(&sect);
            char tmbuf[64];
            strftime(tmbuf, sizeof(tmbuf), "%Y:%m:%d %H:%M:%S", tinfo);

            bld_printf("Num channels : %d\n", num_channels);
            bld_printf("timeStamp    : 0x%016lX %u sec, %u nsec (%s)\n", ptr->timeStamp, sec, nsec, format_ts(sec, nsec).c_str());
            bld_printf("pulseID      : 0x%016lX\n", ptr->pulseID);
            bld_printf("severityMask : 0x%016lX\n", ptr->severityMask);
            bld_printf("version      : 0x%08X\n", ptr->version);

            // Display payload
            if (showData)
                print_data(ptr->signals, num_channels, channel_formats, enabled_channels, ptr->severityMask);
        }

        n -= payloadSize + bldMulticastPacketHeaderSize;

        LOG_VERBOSE("n is %li size of packet=%lu eventData=%lu\n", n, sizeof(bldMulticastPacket_t), sizeof(bldMulticastComplementaryPacket_t));
        bufptr += payloadSize + bldMulticastPacketHeaderSize;
        
        // Display additional events
        int eventNum = 1, isError = 0;
        while (n > 0)
        {
            compptr = (bldMulticastComplementaryPacket_t*)(bufptr);
         
            // Validate event
            auto compSize = bldMulticastComplementaryPacketHeaderSize + payloadSize;
            if ((packetError = validator.validate(compptr, compSize)) != PacketError::None) {
                if (report)
                    report->report_packet_error(packetError, buffer, totalRead);
                printf("Invalid event received: %s, len=%lu\n", to_string(packetError).c_str(), compSize);
                isError = 1;
                break;
            }

            // Skip the event if requested
            if (events.empty() || std::find(events.begin(), events.end(), eventNum) != events.end()) {
                // Compute new timestamp and pulse ID
                uint64_t newTS = compptr->deltaTimeStamp + ptr->timeStamp;
                uint64_t newPulse = compptr->deltaPulseID + ptr->pulseID;
                
                uint32_t sec, nsec;
                extract_ts(newTS, sec, nsec);

                bld_printf("===> event %d\n", eventNum);
                bld_printf("Timestamp     : 0x%016lX %u sec, %u nsec (%s) delta 0x%X\n", newTS, sec, nsec, format_ts(sec, nsec).c_str(), compptr->deltaTimeStamp);
                bld_printf("Pulse ID      : 0x%016lX delta 0x%X\n", newPulse, compptr->deltaPulseID);
                bld_printf("severity mask : 0x%016lX\n", compptr->severityMask);
                if (showData)
                    print_data(compptr->signals, num_channels, channel_formats, enabled_channels, compptr->severityMask);
            }

            n -= compSize;
            bufptr += compSize;

            LOG_VERBOSE("%li bytes remaining\n", n < 0 ? 0 : n);
            eventNum++;
        }

        if (isError)
            continue;

        if (report)
            report->report_packet_recv();

        bld_printf("====== Packet finished ======\n");
    }

    cleanup();

    return 0;
}

/* Handle some cleanup. Write reports and whatnot */
static void cleanup() {
    if (!report)
        return;
    
    std::ofstream stream(reportFile);
    if (!stream.good()) {
        printf("Error while writing report file %s!\n", reportFile);
        return;
    }
    report->serialize(stream);
    printf("Report saved to %s\n", reportFile);
}

static void usage(const char* argv0) {
    printf("USAGE: %s [ARGS]\n", argv0);
    printf("Options:\n");
    for (size_t i = 0; i < arrayLength(long_opts); ++i) {
        char buf[512];
        snprintf(buf, sizeof(buf), "  -%c%s, --%s%s",
            long_opts[i].val,
            long_opts[i].has_arg == no_argument ? "" : " <arg>",
            long_opts[i].name,
            long_opts[i].has_arg == no_argument ? "" : "=<arg>");
        printf("%-30s %s\n", buf, help_text[i]);
    }
    printf("\nUsage examples:\n");
    printf("\n %s -d -b TST:SYS2:4:BLD_PAYLOAD\n    Display BLD packets and data payload\n", argv0);
    printf("\n %s -b TST:SYS2:4:BLD_PAYLOAD -p 3500 -d -c \"0, 3\"\n    Display BLD packets from port 3500 and the data payload for channel 0 and 3\n", argv0);
    printf("\n %s -b TST:SYS2:4:BLD_PAYLOAD -e \"0, 3\" -d\n    Display only event 0 and 3 and their associated data\n", argv0);
    printf("\n %s -b TST:SYS2:4:BLD_PAYLOAD -n 1\n    Display basic info about one BLD packet and exit\n", argv0);
    printf("\n %s -b TST:SYS2:4:BLD_PAYLOAD -d -a 224.0.0.0 -e 0 -n 10 -v 0x10\n    Display event 0's data payload for multicast BLD packets with the version 0x10, and exit after printing 10\n", argv0);
    puts("");
}

static void print_single_channel(int index, uint32_t data, pvxs::TypeCode format, uint64_t sevrMask) {
    printf("  %s raw=0x%08X, ", channel_labels[index].c_str(), data);
    switch(format.code) {
    case pvxs::TypeCode::Float32:
        printf("float=%g", *reinterpret_cast<float*>(&data));
        break;
    case pvxs::TypeCode::Int32:
        printf("int32=%d", *reinterpret_cast<int*>(&data));
        break;
	case pvxs::TypeCode::UInt32A:
    case pvxs::TypeCode::UInt32:
        printf("uint32=%u", data);
        break;
    case pvxs::TypeCode::Int64:
    case pvxs::TypeCode::UInt64:
        printf("int64 not supported");
        break;
    default:
        assert(0);
        break;
    }
    printf(", sevr=%s\n", sevr_to_string(get_sevr(sevrMask, index)));
}

static void print_data(uint32_t* data, size_t num, const std::vector<pvxs::TypeCode::code_t>& formats, const std::vector<int>& channels, uint64_t sevrMask) {
    printf("Data payload:\n");

    // A bit ugly, but we need to pad out the channel formats if num > formats.size()
    std::vector<ChannelType> actualFormats = formats;
    while(actualFormats.size() < num)
        actualFormats.push_back(ChannelType::UInt32);

    if (channels.empty()) {
        for (size_t i = 0; i < num; ++i)
            print_single_channel(i, data[i], actualFormats[i], sevrMask);
    }
    else {
        for (auto chan : channels) {
            if (size_t(chan) >= num)
                continue; // Skip anything we don't have
            print_single_channel(chan, data[chan], actualFormats[chan], sevrMask);
        }
    }
}

static std::vector<ChannelType> parse_channel_formats(const char* str) {
    char buf[512];
    strcpy_safe(buf, str);

    std::vector<ChannelType> fmt;
    for (char* s = strtok(buf, ", "); s; s = strtok(nullptr, ", ")) {
        switch(*s) {
        case 'f':
            fmt.push_back(ChannelType::Float32);
            break;
        case 'i':
            fmt.push_back(ChannelType::Int32);
            break;
        case 'u':
            fmt.push_back(ChannelType::UInt32);
            break;
        default:
            printf("Unknown format '%c'! Valid types are 'f', 'i', and 'u'", *s);
            exit(1);
            return {};
        }
    }
    num_channels = fmt.size();
    return fmt;
}

static std::vector<int> parse_channels(const char* str) {
    char buf[512];
    strcpy_safe(buf, str);

    std::vector<int> channels;
    for (char* s = strtok(buf, ", "); s; s = strtok(nullptr, ", ")) {
        channels.push_back(strtol(s, NULL, 10));
        if (channels.back() < 0 || channels.back() >= NUM_BLD_CHANNELS) {
            printf("Invalid channel index %d!\n", channels.back());
            exit(1);
        }
    }
    std::sort(channels.begin(), channels.end(), [](int a, int b) { return a < b; });
    return channels;
}

static std::vector<int> parse_events(const char* str) {
    char buf[512];
    strcpy_safe(buf, str);

    std::vector<int> evs;
    for (char* s = strtok(buf, ", "); s; s = strtok(nullptr, ", ")) {
        auto ev = strtol(s, nullptr, 10);
        if (ev < 0) {
            printf("Invalid event number %li\n", ev);
            exit(1);
        }
        evs.push_back(ev);
    }
    return evs;
}

static std::vector<ChannelType> read_channel_formats(const char* str) {
    using namespace pvxs;

    auto ctx = client::Context::fromEnv();
    
    LOG_VERBOSE("Read payload PV: %s\n", str);
    Value result;
    try {
        result = ctx.get(str).exec()->wait(10.0);
    }
    catch(pvxs::client::Timeout& e) {
        printf("Timeout while reading PV '%s', please specify channel formats manually with -f\n", str);
        exit(1);
    }

    if (result.type() != TypeCode::Struct) {
        printf("Payload PV '%s' is not of the expected type 'Struct'\n", str);
        exit(1);
    }
    
    auto structure = result["BldPayload"];
    if (!structure.valid()) {
        printf("Payload PV '%s' contains no 'BldPayload' field\n", str);
        exit(1);
    }

    channel_labels.clear();

    std::vector<ChannelType> format;
    int i = 0, chi = 0;
    for (auto ch : structure.ichildren()) {
        format.push_back(ch.type().code);
            
        // Store label for display
        channel_labels.push_back(structure.nameOf(ch));

        // Store off remap
        channel_remap[i] = chi++;
        ++i;
    }
    num_channels = i;
    return format;
}

static void build_channel_list() {
    std::vector<int> chanList;
    
    // Remap user-provided enabled channels to the ones specified by BLD_PAYLOAD
    for (auto c : enabled_channels) {
        assert(c >= 0 && c < NUM_BLD_CHANNELS);
        chanList.push_back(channel_remap[c]);
        LOG_VERBOSE("remap: %d -> %d\n", c, channel_remap[c]);
    }
    enabled_channels = chanList;
}

// Printf helper to disable printing in certain scenarios
// use printf/fprintf directly for things that should always be seen
static void bld_printf(const char* fmt, ...) {
    if ((quiet || report) && !verbose)
        return;

    char msg[65536];
    va_list va;
    va_start(va, fmt);
    vsnprintf(msg, sizeof(msg), fmt, va);
    va_end(va);

    fputs(msg, stdout);
}
