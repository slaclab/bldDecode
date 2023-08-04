
#pragma once

#include <cstdint>
#include <string>

#include <epicsTime.h>

#define CHANNEL_SIZE 4

#if __cplusplus >= 201103L
#define CONSTEXPR constexpr
#else
#define CONSTEXPR static const
#endif

#if defined(__GNUC__) || defined(__clang__)
#define PRINTF_ATTR(x, y) __attribute__((format(printf, x, y)))
#else
#define PRINTF_ATTR(x,y)
#endif

void extract_ts(uint64_t ts, uint32_t& sec, uint32_t& nsec);
std::string format_ts(uint32_t sec, uint32_t nsec);

/**
 * Determine numerical base of the string.
 * Ex: 0x1 => 16, 0b01 => 2, 0o8 => 8
 */
int num_str_base(const char* str);

/**
 * Create an EPICS timestamp from the uint64_t encoded one in a BLD packet
 */
inline epicsTimeStamp epics_from_bld(uint64_t ts) {
    epicsTimeStamp s;
    extract_ts(ts, s.secPastEpoch, s.nsec);
    return s;
}

template<class T, size_t N>
inline constexpr size_t arrayLength(const T (&array)[N]) {
    return N;
}

template<size_t N>
inline void strcpy_safe(char (&dest)[N], const char* s) {
    strncpy(dest, s, N-1);
    dest[N-1] = 0;
}
