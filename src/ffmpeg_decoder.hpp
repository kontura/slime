#pragma once

extern "C" {
    #include <libavutil/frame.h>
    #include <libavutil/mem.h>
    #include <libavcodec/avcodec.h>
    #include <libavutil/opt.h>
    #include <libavutil/imgutils.h>
//for conversion from rgb to yuv
    #include <libswscale/swscale.h>

    #include <libavutil/avassert.h>
    #include <libavutil/channel_layout.h>
    #include <libavutil/mathematics.h>
    #include <libavutil/timestamp.h>
    #include <libavformat/avformat.h>
    #include <libswresample/swresample.h>
}

#include "consts.hpp"

#define AUDIO_INBUF_SIZE 20480
#define AUDIO_REFILL_THRESH 4096

typedef struct ffmpeg_decoder {
    const AVCodec *codec;
    AVCodecContext *context;
    AVCodecParserContext *parser;
    AVPacket *packet;
    AVFrame *decoded_frame;
    uint8_t *inbuf;
    uint8_t *data;
    FILE *infile;
    size_t data_size;
    uint8_t *samples_buffer;
    int samples_buffer_count;
} Decoder;

Decoder *decoder_new(const char *infile);
void decoder_free(Decoder *decoder);
void decode(Decoder *decoder);
size_t load_audio_samples(Decoder *ffmpeg_decoder);
