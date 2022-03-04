#pragma once

extern "C" {
    #include <libavutil/frame.h>
    #include <libavutil/mem.h>
    #include <libavcodec/avcodec.h>
}

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

Decoder *decoder_new();
void decoder_free(Decoder *decoder);
void decode(Decoder *decoder, FILE *outfile);
size_t process_one_read(Decoder *ffmpeg_decoder, FILE *infile, FILE *outfile, size_t data_size);
