#!/bin/bash
set -x

for i in {1..10}; do 
	ts=$(date -u +%s)
	timeout 32 ssh afterchange@192.168.226.175 -C "timeout 32 tshark -w ~/sv_auto/pcap/$ts.pcap 'udp and host 192.168.226.176'" &
	timeout 32 ssh afterchange@192.168.226.175 -C "timeout 32 parec -d 'alsa_output.pci-0000_08_00.4.analog-stereo.monitor' --latency-msec=20 --rate=16000 --file-format=wav ~/sv_auto/audio/$ts.wav" &
	timeout 32 tshark -w ~/sv_auto/pcap/$ts.pcap 'udp and host 192.168.226.175' &
	timeout 32 parec -d 'alsa_output.pci-0000_00_1f.3.analog-stereo.monitor' --latency-msec=20 --rate=16000 --file-format=wav ~/sv_auto/audio/$ts.wav & 

	sleep 1
	timeout 32 ssh afterchange@192.168.226.175 -C "timeout 32 cat /tmp/julius.raw > /tmp/virtmic" &
	timeout 32 cat /tmp/julius.raw > /tmp/virtmic &

	wait

	sleep 5

done
