TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

USR_CXXFLAGS += -std=c++11

#==================================================
# bldDecode

PROD += bldDecode

bldDecode_SRCS += bldDecode.cc
bldDecode_SRCS += util.cc
bldDecode_SRCS += report.cc


bldDecode_LIBS += pvxs Com
INC += bld-proto.h
bldDecode_CFLAGS += -Wall

#==================================================

#==================================================
# bldSend

ifeq ($(BLD_SEND),1)
PROD += bldSend
bldSend_SRCS += bldSend.cc
bldSend_CFLAGS += -Wall
endif

#==================================================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

