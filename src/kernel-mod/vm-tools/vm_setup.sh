#!/bin/sh

# script to be run in a vm to setup a Fastpass endpoint

# disable DHCP
echo "disabling DHCP"
pkill -f dhclient

# configure an IP on the interface
echo "configuring an IP for eth0"
ifconfig eth0 down
ifconfig eth0 up 10.1.1.1
ifconfig eth0

# synchronize time
echo "synchronizing time"
cp ntp.conf /etc/ntp.conf
service ntp stop
ntpdate -d 10.1.1.3
ntpdate -d 10.1.1.3

# clear logs
echo "clearing logs"
./clear-logs.sh
