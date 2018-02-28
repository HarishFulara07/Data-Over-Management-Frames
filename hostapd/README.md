# Wifi-Over-Management-Frames

## Pre-Requisite
1. A WiFi card with AP mode support is needed. Run the command `iw list | grep "Supported interface modes" -A 8` 
to check whether AP mode is supported by your WiFi card. If your WiFi card supports AP mode, then output of this command will look like this -
```
Supported interface modes:
         * IBSS
         * managed
         * AP
         * AP/VLAN
         * WDS
         * monitor
         * P2P-client
         * P2P-GO
```

2. [libnl](https://www.infradead.org/~tgr/libnl/) is required to run hostapd. To install libnl, run the following command in your terminal:
```
sudo apt-get install libnl-genl-3-dev
```

## Configuring hostapd

We need to configure hostpad before running it for the first time.

1. cd into hostapd directory `cd hostapd/`

2. Create the config file `cp defconfig .config`

3. Open the .config file `nano .config`

4. Uncomment the line `CONFIG_LIBNL32=y`. Save and close the config file.

5. Now open hostapd.conf file `nano hostapd.conf`. Following is an example of a simple hostapd.conf file:

```
interface = wlan0
driver = nl80211
hw_mode = g
channel = 1
ssid = testAP
```

6. Save and close hostapd.conf file.

## How to run hostapd

Build with:

```
make
sudo make install
```

Run with: `sudo ./hostapd hostapd.conf`

If you get an error message on console similar to this -

```
Configuration file: hostapd.conf
Could not set channel for kernel driver
Interface initialization failed
wlan0: interface state UNINITIALIZED->DISABLED
wlan0: AP-DISABLED 
wlan0: Unable to setup interface.
wlan0: interface state DISABLED->DISABLED
wlan0: AP-DISABLED 
hostapd_free_hapd_data: Interface wlan0 wasn't started
nl80211: deinit ifname=wlan0 disabled_11b_rates=0
```

then you need to kill you network manager first before running hostapd.

```
sudo service network-manager stop
sudo ./hostapd hostapd.conf 
```
