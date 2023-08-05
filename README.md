
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
  -t <arg>, --timeout=<arg>    Timeout to receive packets
  -n <arg>, --num=<arg>        Number of packets to receive before exiting
  -f <arg>, --format=<arg>     Data format (i.e. 'f,u,i,f' for float, uint32, int32, float)
  -u, --unicast                Receive packets as unicast too
  -c <arg>, --channels=<arg>   Channels to display, comma separated list
  -h, --help                   Display this help text
  -e <arg>, --events=<arg>     Event numbers to display
  -b <arg>, --pv=<arg>         PV that contains a description of the BLD payload
  -v, --verbose                Run in verbose mode, showing additional debugging info
  -a <arg>, --address=<arg>    Multicast address
  -r, --report                 Run in report generation mode
  -o <arg>, --output=<arg>     File to place the generated report
  -q, --quiet                  Disable all non-critical logging
```

bldDecode may be pointed at a `BLD_PAYLOAD` PV to determine the format of the packet using the `-b` parameter:
```
./bldDecode -b TST:SYS2:4:BLD_PAYLOAD
```

By default, all channels are assumed to be in the float32 format. Formats can be manually changed using the `-f` or `--format` argument, however it's recommended to
use `-b` when a PV is available.
