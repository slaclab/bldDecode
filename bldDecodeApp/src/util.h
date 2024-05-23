//////////////////////////////////////////////////////////////////////////////
// This file is part of 'bldDecode'.
// It is subject to the license terms in the LICENSE.txt file found in the 
// top-level directory of this distribution and at: 
//    https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html. 
// No part of 'bldDecode', including this file, 
// may be copied, modified, propagated, or distributed except according to 
// the terms contained in the LICENSE.txt file.
//////////////////////////////////////////////////////////////////////////////
#pragma once

#include <cstdint>
#include <string>

#include <compilerSpecific.h>
#include <epicsTime.h>

#define CHANNEL_SIZE 4

void extract_ts(uint64_t ts, uint32_t& sec, uint32_t& nsec);
std::string format_ts(uint32_t sec, uint32_t nsec);

/**
 * \param mask Severity mask
 * \param channel Channel index
 * \returns Severity
 */
int get_sevr(uint64_t mask, int channel);

/** Returns severity string */
const char* sevr_to_string(int sevr);

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

/**
 * \brief Returns the length of an array. Only works for arrays that the compiler knows the length of at compile time.
 */
template<class T, size_t N>
inline constexpr size_t arrayLength(const T (&array)[N]) {
    return N;
}

/**
 * \brief Safer version of strcpy with inferred destination length. Ensures dest is NULL terminated, even if truncation occurs.
 * prefer this over strncpy!
 */
template<size_t N>
inline void strcpy_safe(char (&dest)[N], const char* s) {
    strncpy(dest, s, N-1);
    dest[N-1] = 0;
}
