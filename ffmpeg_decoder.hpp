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
} Decoder;

// a wrapper around a single output AVStream
typedef struct OutputStream {
    AVStream *st;
    AVCodecContext *enc;

    /* pts of the next frame that will be generated */
    int64_t next_pts;
    int samples_count;

    AVFrame *frame;
    AVFrame *tmp_frame;

    AVPacket *tmp_pkt;

    float t, tincr, tincr2;

    struct SwsContext *sws_ctx;
    struct SwrContext *swr_ctx;
} OutputStream;

typedef struct ffmpeg_encoder {
    OutputStream video_st;
    OutputStream audio_st;
    AVFormatContext *output_context;

    const AVCodec *video_codec;
    const AVCodec *audio_codec;

    AVCodecContext *context;
    AVPacket *packet;
    AVFrame *encoded_frame;
} Encoder;

Decoder *decoder_new();
Encoder *encoder_new(const char *filename);

void decoder_free(Decoder *decoder);
void encoder_free(Encoder *encoder);

size_t decode(Decoder *decoder, FILE *outfile);
size_t encode(Encoder *encoder, FILE *outfile);

size_t process_one_read(Decoder *ffmpeg_decoder, FILE *infile, FILE *outfile, size_t data_size, size_t *written_samples);
