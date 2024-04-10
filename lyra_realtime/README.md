# Compiling and using Lyra Realtime Encoder and Decoder

Before compiling the Lyra Realtime Encoder and Decoder, make sure you have the following dependencies installed
- bazel(use bazelisk to download bazel)
- python3
- numpy

Clone the repository and navigate to the `lyra` directory
```bash
git clone https://github.com/google/lyra.git
cd lyra
```
Apply the patch to the `lyra` directory
```bash
git apply lyra_realtime.patch
```
Compile the encoder and decoder
```bash
bazel build -c opt lyra/cli_example:encoder_realtime
bazel build -c opt lyra/cli_example:decoder_realtime
```
### Local Test using `parec` and `pacat`
```bash
parec --latency-msec=20 --rate=16000 | encoder_realtime | decoder_realtime | pacat --latency-msec=20 --rate=16000 
```
