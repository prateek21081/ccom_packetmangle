# Covert Communication 

People Involved:
- Anirudh S. Kumar
- Prateek Kumar
- Paras Dhiman
- Mehul Arora
- Jithin S.
- Richa Gupta
- Sambuddho Chakravarty

# Project Setup

The test setup contains following machines: 
- Game Server [`GS`]
- Game Client 1 [`C1`]
- Game Client 2 [`C2`]

The objective is to send audio data from `C1` to `C2` using the game traffic via `GS`. To acheive this, the following
components are required on the respective machines:
- iptables rules for intercepting and redirecting game packets to nfqueue [`GS`, `C1`, `C2`]
- Netfilter Queue programs for modifying game packets [`GS`, `C1`, `C2`]
    + `nfqclient` attaches covert data from fifo to outgoing packets
    + `nfqserver` detaches covert data from incoming packets to fifo
    + each program ensures the packet integrity (recalculate payload checksums and lengths)
- Audio transcoders for recording and playback [`C1`, `C2`]
    + `encoder_realtime` reads raw audio samples from `stdin` and writes encoded audio to `stdout`
    + `decoder_realtime` reads encoded audio from `stdout` and writes raw audio samples to `stdin`
- FIFOs to create buffers and connect the nfqueue programs and audio transcoders


# How to Setup?

Perform the following steps on each machine in order to have a working setup:

### `C1`
1. Add an iptables rule to intercept all the outgoing game packets towards `GS`.
    + `iptables -I OUTPUT -d <server_addr> -s <client_addr> -j NFQUEUE --queue_num <qnum>`
2. Start recording raw audio from the microphone, encode the audio and write that a fifo.
    + `parec --latency-msec=20 --rate=16000 | encoder_realtime > <fifo_path>`
3. Start nfqclient to attach some bytes from the fifo to the outgoing packets towards `GS`. 
    + `nfqclient <fifo_path> <qnum> <max_bytes_per_packet>`

### `GS`
1. Add an iptables rule to intercept all the incoming game packets from `C1`.
    + `iptables -I INPUT -d <GS_addr> -s <C1_addr> -j NFQUEUE --queue_num <qnum_a>`
2. Start nfqserver to extract bytes from the packet and write them to a fifo.
    + `nfqserver <fifo_path> <qnum_a> <max_bytes_per_packet>`
3. Add an iptables rule to intercept all the outgoing game packets towards `C2`.
    + `iptables -I OUTPUT -d <C2_addr> -s <GS_addr> -j NFQUEUE --queue_num <qnum_b>`
4. Start nfqclient to attach some bytes from the fifo to the outgoing packets towards `C2`. 
    + `nfqclient <fifo_path> <qnum_b> <max_bytes_per_packet>`

### `C2`
1. Add an iptables rule to intercept all the incoming game packets from `GS`.
    + `iptables -I INPUT -d <GS_addr> -s <C2_addr> -j NFQUEUE --queue_num <qnum>`
2. Start nfqserver to extract bytes from the packets and write them to a fifo.
    + `nfqserver <fifo_path> <qnum> <max_bytes_per_packet>`
3. Read encoded audio form the fifo, decode the audio and play it.
    + `cat <fifo_path> | decoder_realtime | pacat --latency-msec=20 --rate=16000`
