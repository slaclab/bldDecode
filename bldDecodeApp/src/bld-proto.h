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

#include <stdint.h>

#define DEFAULT_BLD_PORT 50000
#define NUM_BLD_CHANNELS 31
#define BLD_CHANNEL_SIZE sizeof(uint32_t)

typedef struct __attribute__((__packed__)) {
    uint64_t timeStamp;
    uint64_t pulseID;
    uint32_t version;
    uint64_t severityMask;
    uint32_t signals[NUM_BLD_CHANNELS];
} bldMulticastPacket_t;

const int bldMulticastPacketHeaderSize = (sizeof(bldMulticastPacket_t) - sizeof(uint32_t) * NUM_BLD_CHANNELS);

typedef struct __attribute__((__packed__)) {
	uint32_t deltaTimeStamp:20;
	uint32_t deltaPulseID:12;
	uint64_t severityMask;
	uint32_t signals[NUM_BLD_CHANNELS];
} bldMulticastComplementaryPacket_t;


const int bldMulticastComplementaryPacketHeaderSize = (sizeof(bldMulticastComplementaryPacket_t) - sizeof(uint32_t) * NUM_BLD_CHANNELS);

#define BLD_PACKET_NUM_CHANNELS(_ptr, _packSize) ((_packSize - (sizeof(bldMulticastPacket_t) - sizeof(_ptr->signals))) / BLD_CHANNEL_SIZE);