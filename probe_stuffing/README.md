# Wifi Over Management Frames - Probe Stuffing

## Pre-Requisite
[libnl](https://www.infradead.org/~tgr/libnl/) is required to run the code. To install libnl, run the following command in your terminal:
```
sudo apt-get install libnl-genl-3-dev
```
## How to run
**Note**: The code only works on network interfaces whose drivers are compatible with Netlink. Test this by running `iw list` command in your terminal. If your network interface shows up in the output, then it is compatible with Netlink.

**Note**: Root privileges are required to run the compiled program.

Build with: `make`

Run with: `sudo ./probe_stuffing <Network Interface> <Data>`

`<Network Interface>` The network interface to use for sending scan request. Data is stuffed inside the probe request packets sent during the scan request.

`<Data>` The data to stuff inside the probe request packet. The data can be max 252 characters long. 

## Resources:
[1] http://git.kernel.org/cgit/linux/kernel/git/jberg/iw.git/tree/scan.c

[2] http://stackoverflow.com/questions/21601521/how-to-use-the-libnl-library-to-trigger-nl80211-commands

[3] https://stackoverflow.com/questions/23760780/how-to-send-single-channel-scan-request-to-libnl-and-receive-single-channel-sca
