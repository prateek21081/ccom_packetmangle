#include <iostream>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
}

int main(int argc, char* argv[]) {
    // Initialize the ffmpeg libraries
    av_register_all();
    avcodec_register_all();

    // Open the input audio file
    AVFormatContext* input_ctx = nullptr;
    if (avformat_open_input(&input_ctx, argv[1], nullptr, nullptr) < 0) {
        std::cerr << "Error: could not open input file" << std::endl;
        return 1;
    }
    if (avformat_find_stream_info(input_ctx, nullptr) < 0) {
        std::cerr << "Error: could not find input stream information" << std::endl;
        return 1;
    }

    // Find the audio stream in the input file
    int audio_stream_index = -1;
    for (unsigned int i = 0; i < input_ctx->nb_streams; i++) {
        if (input_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream_index = i;
            break;
        }
    }
    if (audio_stream_index == -1) {
        std::cerr << "Error: could not find audio stream in input file" << std::endl;
        return 1;
    }

    // Get the input audio codec parameters
    AVCodecParameters* input_codecpar = input_ctx->streams[audio_stream_index]->codecpar;

    // Find the Opus audio codec
    AVCodec* output_codec = avcodec_find_encoder_by_name("libopus");
    if (!output_codec) {
        std::cerr << "Error: could not find Opus codec" << std::endl;
        return 1;
    }

    // Create a new output audio stream
    AVFormatContext* output_ctx = nullptr;
    if (avformat_alloc_output_context2(&output_ctx, nullptr, "opus", nullptr) < 0) {
        std::cerr << "Error: could not create output context" << std::endl;
        return 1;
    }
    AVStream* output_stream = avformat_new_stream(output_ctx, output_codec);
    if (!output_stream) {
        std::cerr << "Error: could not create output stream" << std::endl;
        return 1;
    }

    // Copy the input audio codec parameters to the output stream
    if (avcodec_parameters_copy(output_stream->codecpar, input_codecpar) < 0) {
        std::cerr << "Error: could not copy codec parameters" << std::endl;
        return 1;
    }

    // Set the output audio codec parameters
    output_stream->codecpar->codec_id = output_codec->id;
    output_stream->codecpar->codec_type = AVMEDIA_TYPE_AUDIO;
    output_stream->codecpar->bit_rate = 128000;
    output_stream->codecpar->channels = 2;
    output_stream->codecpar->channel_layout = AV_CH_LAYOUT_STEREO;
    output_stream->codecpar->sample_rate = 44100;
    output_stream->codecpar->format = AV_SAMPLE_FMT_FLTP;

    // Open the output audio codec
    if (avcodec_open2(output_stream->codec, output_codec, nullptr) < 0) {
        std::cerr << "Error: could not open output codec" << std::endl;
        return 1;
    }

    // Open the output file
    if (!(output_ctx->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&output_ctx->pb, "-", AVIO_FLAG_WRITE) < 0) {
            std::cerr << "Error: could not open output file" << std::endl;
            return 1;
        }
    }

    // Write the output file header
    if (avformat_write_header(output_ctx, nullptr) < 0) {
        std::cerr << "Error: could not write output file header" << std::endl;
        return 1;
    }

    // Initialize the audio resampler
    SwrContext* resampler = swr_alloc_set_opts(nullptr,
        av_get_default_channel_layout(output_stream->codecpar->channels),
        output_stream->codecpar->format,
        output_stream->codecpar->sample_rate,
        av_get_default_channel_layout(input_codecpar->channels),
        (AVSampleFormat)input_codecpar->format,
        input_codecpar->sample_rate,
        0, nullptr);
    if (!resampler) {
        std::cerr << "Error: could not initialize audio resampler" << std::endl;
        return 1;
    }
    if (swr_init(resampler) < 0) {
        std::cerr << "Error: could not initialize audio resampler" << std::endl;
        return 1;
    }

    // Allocate input and output audio frames
    AVFrame* input_frame = av_frame_alloc();
    AVFrame* output_frame = av_frame_alloc();
    if (!input_frame || !output_frame) {
        std::cerr << "Error: could not allocate audio frames" << std::endl;
        return 1;
    }

    // Read input audio packets and encode them to Opus
    AVPacket output_packet;
    av_init_packet(&output_packet);
    output_packet.data = nullptr;
    output_packet.size = 0;
    while (true) {
        // Read the next input audio packet
        AVPacket input_packet;
        av_init_packet(&input_packet);
        input_packet.data = nullptr;
        input_packet.size = 0;
        if (av_read_frame(input_ctx, &input_packet) < 0) {
            break;
        }
        if (input_packet.stream_index != audio_stream_index) {
            av_packet_unref(&input_packet);
            continue;
        }

        // Decode the input audio packet
        int ret = avcodec_send_packet(input_stream->codec, &input_packet);
        if (ret < 0) {
            std::cerr << "Error: could not send input packet for decoding" << std::endl;
            av_packet_unref(&input_packet);
            break;
        }
        while (ret >= 0) {
            ret = avcodec_receive_frame(input_stream->codec, input_frame);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break;
            }
            if (ret < 0) {
                std::cerr << "Error: could not decode input frame" << std::endl;
                av_packet_unref(&input_packet);
                break;
            }

            // Resample the input audio frame
            output_frame->nb_samples = input_frame->nb_samples;
            output_frame->format = output_stream->codecpar->format;
            output_frame->channel_layout = output_stream->codecpar->channel_layout;
            av_frame_get_buffer(output_frame, 0);
            swr_convert(resampler, output_frame->data, output_frame->nb_samples,
                (const uint8_t**)input_frame->data, input_frame->nb_samples);

            // Encode the output audio frame to Opus
            ret = avcodec_send_frame(output_stream->codec, output_frame);
            if (ret < 0) {
                std::cerr << "Error: could not send output frame for encoding" << std::endl;
                av_packet_unref(&input_packet);
                break;
            }
            while (ret >= 0) {
                ret = avcodec_receive_packet(output_stream->codec, &output_packet);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    break;
                }
                if (ret < 0) {
                    std::cerr << "Error: could not encode output packet" << std::endl;
                    av_packet_unref(&input_packet);
                    break;
                }

                // Write the output packet to the output file
                output_packet.stream_index = output_stream->index;
                av_packet_rescale_ts(&output_packet, output_stream->time_base, output_stream->time_base);
                av_interleaved_write_frame(output_ctx, &output_packet);
                av_packet_unref(&output_packet);
            }
        }
        av_packet_unref(&input_packet);
    }

    // Write the output file trailer
    av_write_trailer(output_ctx);

    // Cleanup
    avcodec_free_context(&output_stream->codec);
    avformat_close_input(&input_ctx);
    avformat_free_context(output_ctx);
    av_frame_free(&input_frame);
    av_frame_free(&output_frame);
    swr_free(&resampler);

    return 0;
}