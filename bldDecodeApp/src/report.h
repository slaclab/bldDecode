
#pragma once

#include <fstream>
#include <list>
#include <memory>
#include <cstdlib>
#include <cstring>
#include <cstdint>

#include <epicsTime.h>

#include "bld-proto.h"

class Report;

enum class PacketError {
    None,
    Unknown,
    BadHeader,        // Invalid header
    BadTimestamp,     // Timestamp is located TIMESTAMP_EPSILON seconds before the first received packet's timestamp, and is likely garbage
    BadEvent,         // Event is invalid in some way. Too short, missing channels, etc.
};

std::string to_string(PacketError reason);

/** Epsilon (in seconds) for timestamp validation */
/** If the packet timestamp is this many seconds behind the first recv'ed packets timestamp, it is considered invalid */
constexpr double TIMESTAMP_EPSILON = 60.0;

/**
 * Validator for BLD packets
 * can be used independently of the rest of the reporting infrastructure
 */
class PacketValidator {
public:
    /**
     * \brief Validate a BLD packet header
     * \param packet Pointer to the packet header
     * \param datalen Length of the header (NOTE: this is NOT the total size of the recv'ed data, only of the header + its signals)
     */
    PacketError validate(bldMulticastPacket_t* packet, size_t datalen);

    /**
     * \brief Validate a complementary (aka event) packet
     * \param packet Pointer to the complementary packet header
     * \param datalen Size of the complementary packet (This is NOT the total size of the packet, only of this single event)
     */
    PacketError validate(bldMulticastComplementaryPacket_t* packet, size_t datalen);

private:
    epicsTimeStamp m_firstTimestamp;
    bool m_hasFirstTimestamp = false;
};

/**
 * A single entry in the report, detailing an error and its data
 */
class ReportEntry {
public:
    ReportEntry(PacketError error, const void* data, size_t datalen, uint64_t index, const epicsTimeStamp& recvAt) :
        m_dataLen(datalen),
        m_index(index),
        m_recvTime(recvAt),
        m_reason(error)
    {
        m_data = malloc(datalen);
        memcpy(m_data, data, datalen);
    }

    ReportEntry(const ReportEntry& rhs) = delete;
    ReportEntry() = delete;

    ~ReportEntry() { free(m_data); }

    std::string to_string() const;

    inline uint64_t index() const { return m_index; }
    inline size_t data_length() const { return m_dataLen; }
    inline epicsTimeStamp recv_time() const { return m_recvTime; }
    inline PacketError reason() const { return m_reason; }

private:
    void* m_data;
    size_t m_dataLen;
    uint64_t m_index;
    epicsTimeStamp m_recvTime;
    PacketError m_reason;
};

/**
 * Report container class
 * Maintains a memory bounded list of errors
 */
class Report {
public:
    /**
     * Report that a valid BLD packet has been recv'ed
     */
    void report_packet_recv() {
        ++m_totalPackets;
    }

    /**
     * Report an invalid packet with a reason
     */
    void report_packet_error(PacketError reason, const void* data, size_t dataLen) {
        epicsTimeStamp s;
        epicsTimeGetCurrent(&s);
        m_entries.emplace_back(reason, data, dataLen, m_totalPackets, s);
        ++m_errorPackets;
        ++m_totalPackets;
    }

    void serialize(std::ofstream& stream);

private:
    std::list<ReportEntry> m_entries;
    uint64_t m_totalPackets = 0;
    uint64_t m_errorPackets = 0;
};
