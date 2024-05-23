
# bldDecode

`bldDecode` is a simple test application used to decode and inspect BLD packets.

## Requirements

* EPICS Base
* [pvxs](https://github.com/mdavidsaver/pvxs)

## Compiling

bldDecode uses the EPICS build system. 
* Set up a `RELEASE_SITE` or `RELEASE.local` file describing your EPICS environment and the location of the pvxs module. 
* Run `make`

## Usage

```
USAGE: bldDecode [ARGS]
Options:
  -p <arg>, --port=<arg>       The port to use (default: 50000)
  -d, --show-data              Display event data
  -k <arg>, --version=<arg>    Filter packets by this version
  -s <arg>, --severity=<arg>   Filter packets by this severity mask
  -t <arg>, --timeout=<arg>    Timeout to receive packets, in seconds
  -n <arg>, --num=<arg>        Number of packets to receive before exiting
  -f <arg>, --format=<arg>     Data format (i.e. 'f,u,i,f' for float, uint32, int32, float)
  -u, --unicast                Receive packets as unicast too
  -c <arg>, --channels=<arg>   Channels to display (i.e. '1,2,5' will display channels 1, 2 and 5)
  -h, --help                   Display this help text
  -e <arg>, --events=<arg>     Event indices to display (i.e. '0,1,3' will display events 0, 1 and 3)
  -b <arg>, --pv=<arg>         PV that contains a description of the BLD payload
  -v, --verbose                Run in verbose mode, showing additional debugging info
  -a <arg>, --address=<arg>    Multicast address
  -r, --report                 Run in report generation mode
  -o <arg>, --output=<arg>     File to place the generated report
  -q, --quiet                  Disable all non-critical logging

Usage examples:

 ./bin/linux-x86_64/bldDecode -d -b TST:SYS2:4:BLD_PAYLOAD
    Display BLD packets and data payload

 ./bin/linux-x86_64/bldDecode -b TST:SYS2:4:BLD_PAYLOAD -p 3500 -d -c "0, 3"
    Display BLD packets from port 3500 and the data payload for channel 0 and 3

 ./bin/linux-x86_64/bldDecode -b TST:SYS2:4:BLD_PAYLOAD -e "0, 3" -d
    Display only event 0 and 3 and their associated data

 ./bin/linux-x86_64/bldDecode -b TST:SYS2:4:BLD_PAYLOAD -n 1
    Display basic info about one BLD packet and exit

 ./bin/linux-x86_64/bldDecode -b TST:SYS2:4:BLD_PAYLOAD -d -a 224.0.0.0 -e 0 -n 10 -v 0x10
    Display event 0's data payload for multicast BLD packets with the version 0x10, and exit after printing 10


```

bldDecode may be pointed at a `BLD_PAYLOAD` PV to determine the format of the packet using the `-b` parameter:
```
./bldDecode -b TST:SYS2:4:BLD_PAYLOAD
```

By default, all channels are assumed to be in the float32 format. Formats can be manually changed using the `-f` or `--format` argument, however it's recommended to
use `-b` when a PV is available.
