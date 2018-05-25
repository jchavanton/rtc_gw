#!/bin/sh



echo 10 > /proc/sys/net/ipv4/tcp_fin_timeout
echo 1  > /proc/sys/net/ipv4/tcp_tw_recycle
echo 1  > /proc/sys/net/ipv4/tcp_tw_reuse

cd ../..
./out/Default/rtc_gw --port 9999 --listen 147.75.38.39
