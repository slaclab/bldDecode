//////////////////////////////////////////////////////////////////////////////
// This file is part of 'bldDecode'.
// It is subject to the license terms in the LICENSE.txt file found in the 
// top-level directory of this distribution and at: 
//    https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html. 
// No part of 'bldDecode', including this file, 
// may be copied, modified, propagated, or distributed except according to 
// the terms contained in the LICENSE.txt file.
//////////////////////////////////////////////////////////////////////////////
#include "report.h"
#include "util.h"

#include <cassert>

constexpr const char BASE64_TABLE[] =
    "ABCDEFGHIJKLMNOPQRSTUVXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// Encode packet data in base64
std::string ReportEntry::to_string() const {
    auto *data = static_cast<char *>(m_data);

    std::string str;
    str.resize(m_dataLen * 4 + 1);
    for (int i = 0; i < m_dataLen;) {
        auto a = i < m_dataLen ? (unsigned char)data[i++] : 0;
        auto b = i < m_dataLen ? (unsigned char)data[i++] : 0;
        auto c = i < m_dataLen ? (unsigned char)data[i++] : 0;

        uint32_t triple = (a << 0x10) + (b << 0x08) + c;

        str.push_back(BASE64_TABLE[(triple >> 3 * 6) & 0x3F]);
        str.push_back(BASE64_TABLE[(triple >> 2 * 6) & 0x3F]);
        str.push_back(BASE64_TABLE[(triple >> 1 * 6) & 0x3F]);
        str.push_back(BASE64_TABLE[triple & 0x3F]);
    }
    return str;
}

std::string to_string(PacketError reason) {
    switch(reason) {
    case PacketError::BadEvent:
        return "Invalid event";
    case PacketError::BadHeader:
        return "Invalid header";
    case PacketError::BadTimestamp:
        return "Invalid timestamp";
    case PacketError::Unknown:
    default:
        return "Unknown";
    }
}

static std::string to_string(const epicsTimeStamp& ts) {
    char time[256];
    epicsTimeToStrftime(time, sizeof(time), "%a %b %d %Y %H:%M:%S.%09f", &ts);
    return time;
}

void Report::serialize(std::ofstream& stream) {
    stream << "{\n";
    stream << "\t\"recv\": " << m_totalPackets << ",\n";
    stream << "\t\"errors\": " << m_errorPackets << ",\n";
    stream << "\t\"errorPackets\": [\n";

    bool first = true;
    for (auto& packet : m_entries) {
        if (!first)
            stream << ",\n";
        stream << "\t\t{\n";
        stream << "\t\t\t\"index\": " << packet.index() << ",\n";
        stream << "\t\t\t\"size\": " << packet.data_length() << ",\n";
        stream << "\t\t\t\"reason\": " << to_string(packet.reason()) << ",\n";
        stream << "\t\t\t\"time\": " << to_string(packet.recv_time()) << ",\n";
        stream << "\t\t\t\"time_raw\": " << double(packet.recv_time().secPastEpoch) + (packet.recv_time().nsec / 1e9) << ",\n";
        stream << "\t\t\t\"data\": " << packet.to_string() << "\n";

        if (first)
            first = false;
        stream << "\t\t}\n";
    }
    stream << "\t]\n}" << std::endl;
}


PacketError PacketValidator::validate(bldMulticastPacket_t* packet, size_t datalen) {
    if (datalen < bldMulticastPacketHeaderSize)
        return PacketError::BadHeader;

    if (!m_hasFirstTimestamp) {
        uint32_t sec, nsec;
        extract_ts(packet->timeStamp, sec, nsec);
        m_firstTimestamp.nsec = nsec;
        m_firstTimestamp.secPastEpoch = sec;
        m_hasFirstTimestamp = true;
    }
    // Validate timestamp...
    else {
        auto newts = m_firstTimestamp;
        epicsTimeAddSeconds(&newts, -TIMESTAMP_EPSILON);
        auto packetTs = epics_from_bld(packet->timeStamp);
        if (epicsTimeLessThan(&packetTs, &newts))
            return PacketError::BadTimestamp;
    }

    return PacketError::None;
}

PacketError PacketValidator::validate(bldMulticastComplementaryPacket_t* packet, size_t datalen) {
    assert(m_hasFirstTimestamp);

    if (datalen < bldMulticastComplementaryPacketHeaderSize)
        return PacketError::BadEvent;

    return PacketError::None;
}