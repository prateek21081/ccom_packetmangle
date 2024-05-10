#!/usr/bin/env bash

mode=""

relay_addr="127.0.0.1"
sender_addr="127.0.0.1"
receiver_addr="127.0.0.1"

queue_num_c1=0
queue_num_c2=1

buffer_path="/tmp/ccom_buffer_$RANDOM"
payload_length=32

create_buffer() {
    if [[ ! -e $buffer_path ]]; then
        mkfifo $buffer_path
        chmod 666 $buffer_path
    fi
}

handle_sigint() {
    echo "Cleaning up..."
    pkill nfq* 
    pkill parec; pkill pacat
    iptables -F
    rm -f /tmp/ccom_buffer_*
    echo "Exiting..."
}
trap handle_sigint SIGINT

if [[ "$1" == "send" || "$1" == "recv" || "$1" == "copy" ]]; then
    mode=$1; shift;
else
    echo "usage: $0 {send|recv|copy} [ARGS]"
    exit
fi

while [[ "$1" =~ ^- && ! "$1" == "--" ]]; do case $1 in
    -s | --sender_addr )
        shift; sender_addr=$1
    ;;
    -r | --relay_addr )
        shift; relay_addr=$1
    ;;
    -d | --receiver_addr )
        shift; receiver_addr=$1
    ;;
    -b | --buffer_path )
        shift; buffer_path=$1
    ;;
    -l | --payload_length )
        shift; payload_length=$1
    ;;
    *)
        echo "error: unknown option $1"    
        exit
    ;;
esac; shift; done
if [[ "$1" == '--' ]]; then shift; fi

echo "Starting in $mode mode..."
if [[ $mode == "send" ]]; then
    create_buffer
    echo "Starting lyra encoder..."
    pulseaudio 2&>1 & sleep 2
    parec --latency-msec=20 --rate=16000 | encoder_realtime --model_path=$LYRA_COEFF > $buffer_path &
    echo "Adding iptables OUTPUT rule..."
    iptables -I OUTPUT -s $sender_addr -d $relay_addr -p udp -j NFQUEUE --queue-num $queue_num_c1
    iptables -L OUTPUT
    echo "Starting nfqclient..."
    nfqclient $buffer_path $queue_num_c1 $payload_length
elif [[ $mode == "copy" ]]; then
    create_buffer
    echo "Adding iptables rules..."
    iptables -I INPUT -s $sender_addr -d $relay_addr -p udp -j NFQUEUE --queue-num $queue_num_c1
    iptables -I OUTPUT -s $relay_addr -d $receiver_addr -p udp -j NFQUEUE --queue-num $queue_num_c2
    iptables -L 
    echo "Starting nfqclient & nfqserver..."
    nfqclient $buffer_path $queue_num_c2 $payload_length &
    nfqserver $buffer_path $queue_num_c1 $payload_length
elif [[ $mode == "recv" ]]; then
    create_buffer
    echo "Starting lyra decoder..."
    pulseaudio 2&>1 & sleep 2
    cat $buffer_path | decoder_realtime --model_path=$LYRA_COEFF | pacat --latency-msec=20 --rate=16000 &
    echo "Adding iptables INPUT rule..."
    iptables -I INPUT -s $relay_addr -d $receiver_addr -p udp -j NFQUEUE --queue-num $queue_num_c2
    iptables -L INPUT
    echo "Starting nfqserver..."
    nfqserver $buffer_path $queue_num_c2 $payload_length
    echo
fi
