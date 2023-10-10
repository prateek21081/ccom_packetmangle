import pyaudio
import opuslib
import time

# Set up audio capture
audio = pyaudio.PyAudio()
stream = audio.open(format=pyaudio.paInt16, channels=1, rate=48000, input=True, frames_per_buffer=960)

# Set up Opus encoding
encoder = opuslib.Encoder(48000, 1, opuslib.APPLICATION_VOIP)
output_file = open('output.opus', 'wb')

# Continuously capture and encode audio
while True:
    try:
        data = stream.read(960)
        encoded_data = encoder.encode(data, 960)
        output_file.write(encoded_data)
    except KeyboardInterrupt:
        break

# Clean up
output_file.close()
stream.stop_stream()
stream.close()
audio.terminate()

# Wait for output file to be fully written
time.sleep(1)

# Set up audio playback
audio = pyaudio.PyAudio()
stream = audio.open(format=pyaudio.paInt16, channels=1, rate=48000, output=True)
input_file = open('output.opus', 'rb')

# Set up Opus decoding
decoder = opuslib.Decoder(48000, 1)

# Continuously read and play back audio
while True:
    data = input_file.read(4096)
    if not data:
        break
    decoded_data = decoder.decode(data, 960)
    stream.write(decoded_data)

# Clean up
input_file.close()
stream.stop_stream()
stream.close()
audio.terminate()