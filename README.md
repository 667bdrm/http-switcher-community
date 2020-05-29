## http-switcher

http-switcher is a simple home automation ESP8266 firmware. Once configured you could obtain data from up to 4 OneWire temperature sensors and switch on/off devices connected to GPIO ports. Based on Sming Framework 3. Tested for a years for remotely controlling house heater, 1 reboot per year (I think the reason is unstable DC power), no 911 called :)

## Requirements

1. esp-open-sdk 1.5.4 (c70543e57fb18e5be0315aa217bca27d0e26d23d) 
https://github.com/pfalcon/esp-open-sdk.git

2. Sming Framework 3.xx (e13f10f95650c31faf584c71c2646f5312976909
https://github.com/SmingHub/Sming.git)

## Building

1. Install required tools
2. Connect ESP8266 to UART
3. Configure project to match your hardware if needed
4. make flash

## GPIO pinout

4 - OneWire bus

5, 12, 13, 14, 16 - on/off switches

## Configuration

1. Connect to device access point
2. Set your networking connection access credentials
3. Set configuration access point SSID and PSK

## Usage

Replace 192.168.1.100 with device ip obtained from your wireless network

ON: 

curl -d gpio=5 -d set_value=0 http://192.168.1.100/ajax/sensors


OFF: 

curl -d gpio=5 -d set_value=1 http://192.168.1.100/ajax/sensors


Get data:

curl http://192.168.1.100/ajax/sensors

or open http://192.168.1.100/ in browser


## !!! WARNING !!!

This device *DOES NOT* authenticate control messages. Use it in trusted network or protect control channel by encryption or make pull request with the security improvement feature.


## Author and License

Copyright (C) 2016 667bdrm
Licensed under GNU Lesser General Public License v3.0
