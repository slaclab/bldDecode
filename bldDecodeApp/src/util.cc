//////////////////////////////////////////////////////////////////////////////
// This file is part of 'bldDecode'.
// It is subject to the license terms in the LICENSE.txt file found in the 
// top-level directory of this distribution and at: 
//    https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html. 
// No part of 'bldDecode', including this file, 
// may be copied, modified, propagated, or distributed except according to 
// the terms contained in the LICENSE.txt file.
//////////////////////////////////////////////////////////////////////////////
#include "util.h"

#include <time.h>
#include <epicsTime.h>

void extract_ts(uint64_t ts, uint32_t& sec, uint32_t& nsec) {
    nsec = ts & 0xFFFFFFFF;
    sec = (ts>>32) & 0xFFFFFFFF;
}

std::string format_ts(uint32_t sec, uint32_t nsec) {
    time_t sect;
    epicsTimeStamp ts = { sec, nsec };
    epicsTimeToTime_t(&sect, &ts);
    auto tinfo = localtime(&sect);
    
    static char tmbuf[128] {};
    strftime(tmbuf, sizeof(tmbuf), "%Y:%m:%d %H:%M:%S", tinfo);
    return tmbuf;
}

int num_str_base(const char* str) {
    if (str[0] == '0') {
        switch(str[1]) {
        case 'x':
            return 16;
        case 'b':
            return 2;
        case 'o':
            return 8;
        default:
            return 10;
        }
    }
    return 10;
}

int get_sevr(uint64_t mask, int channel) {
    return (mask >> (2*channel)) & 0x3;
}

const char* sevr_to_string(int sevr) {
    switch(sevr) {
    case 0:
        return "None";
    case 1:
        return "Minor";
    case 2:
        return "Major";
    default:
        return "Invalid";
    }
}