#!/bin/bash
set -x

for i in {1..3}; do 
	timeout 30 ssh afterchange@192.168.226.175 -C "timeout 30 tshark -w ~/sv_auto/pcap/$i.pcap 'tcp and host 192.168.226.176'" &
	timeout 30 ssh afterchange@192.168.226.175 -C "timeout 30 parec -d 'alsa_output.pci-0000_08_00.4.analog-stereo.monitor' --latency-msec=20 --rate=16000 --file-format=wav ~/sv_auto/audio/$i.wav" &
	timeout 30 ssh afterchange@192.168.226.175 -C "timeout 30 cat /tmp/julius.raw > /tmp/virtmic" &

	timeout 30 tshark -w ~/sv_auto/pcap/$i.pcap 'tcp and host 192.168.226.175' &
	timeout 30 parec -d 'alsa_output.pci-0000_00_1f.3.analog-stereo.monitor' --latency-msec=20 --rate=16000 --file-format=wav ~/sv_auto/audio/$i.wav & 
	timeout 30 cat /tmp/julius.raw > /tmp/virtmic &
	wait

	sleep 5

done
