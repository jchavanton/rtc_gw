#!/bin/sh

echo 10 > /proc/sys/net/ipv4/tcp_fin_timeout
echo 1  > /proc/sys/net/ipv4/tcp_tw_recycle
echo 1  > /proc/sys/net/ipv4/tcp_tw_reuse
cd ../..
IP=`ip addr | grep 'state UP' -A2 | tail -n1 | awk '{print $2}' | cut -f1  -d'/'`
CMD="./out/Default/rtc_gw --port 9999 --listen $IP"
echo $CMD
exec $CMD
