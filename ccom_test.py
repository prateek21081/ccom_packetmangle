import os, time
receiver_hostname = "afterchange"
receiver_ip = "192.168.226.175"
ssh_cmd = f"ssh {receiver_hostname}@{receiver_ip}"
duration = 30
audio_path = "/tmp/julius.wav"
sender_fifo_path = "/tmp/ccom_send"
reciever_fifo_path = "/tmp/ccom_recv"

sample_rate = 16000

for i in range(1, 121):
    print(time.time())
    sender_audio_cmd = f"cat {audio_path} | encoder_realtime --model_path=$LYRA_COEFF --sample_rate_hz={sample_rate} --enable_dtx=false > {sender_fifo_path} &"
    reciever_audio_cmd = f"timeout {duration}s cat {reciever_fifo_path} | decoder_realtime --model_path=$LYRA_COEFF --sample_rate_hz={sample_rate} > ~/sv_audio/{i}.raw &"
    tshark_cmd = f"tshark -f 'udp and (host 192.168.226.175 or host 192.168.226.176)' -a duration:{duration} -w ~/sv_capture/{i}.pcap"
    print(sender_audio_cmd)
    print(reciever_fifo_path)
    print(tshark_cmd)

    # start pcap capture
    os.system(tshark_cmd.format(duration=duration, i=i) + " &")
    print(tshark_cmd.format(duration=duration, i=i) + " &")
    ssh_tshark_cmd = tshark_cmd.format(duration=duration, i=i)
    print(ssh_tshark_cmd)
    os.system(f"{ssh_cmd} \"{ssh_tshark_cmd}\"" + "&")

    time.sleep(1)
    print(sender_audio_cmd)
    print(reciever_audio_cmd)
    time.sleep(duration+1)
    print(f"Done {i}")
