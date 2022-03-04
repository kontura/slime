#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ffmpeg_decoder.hpp"

Decoder *decoder_new() {
    Decoder *decoder = (Decoder *) malloc(sizeof(Decoder));

    decoder->packet = av_packet_alloc();

    decoder->codec = avcodec_find_decoder(AV_CODEC_ID_MP2);
    if (!decoder->codec) {
        fprintf(stderr, "Codec not found\n");
        exit(1);
    }

    decoder->parser = av_parser_init(decoder->codec->id);
    if (!decoder->parser) {
        fprintf(stderr, "Parser not found\n");
        exit(1);
    }

    decoder->context = avcodec_alloc_context3(decoder->codec);
    if (!decoder->context) {
        fprintf(stderr, "Could not allocate audio codec context\n");
        exit(1);
    }

    /* open it */
    if (avcodec_open2(decoder->context, decoder->codec, NULL) < 0) {
        fprintf(stderr, "Could not open codec\n");
        exit(1);
    }

    if (!(decoder->decoded_frame = av_frame_alloc())) {
        fprintf(stderr, "Could not allocate audio frame\n");
        exit(1);
    }

    decoder->inbuf = (uint8_t *) malloc(sizeof(uint8_t) * AUDIO_INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE);
    decoder->data = decoder->inbuf;

    return decoder;
}

void decoder_free(Decoder *decoder) {
    avcodec_free_context(&(decoder->context));
    av_parser_close(decoder->parser);
    av_frame_free(&(decoder->decoded_frame));
    av_packet_free(&(decoder->packet));
    free(decoder->inbuf);
    free(decoder);
}

void decode(Decoder *decoder, FILE *outfile) {
    int i, ch;
    int ret, data_size;

    /* send the packet with the compressed data to the decoder */
    ret = avcodec_send_packet(decoder->context, decoder->packet);
    if (ret < 0) {
        fprintf(stderr, "Error submitting the packet to the decoder\n");
        exit(1);
    }

    /* read all the output frames (in general there may be any number of them */
    while (ret >= 0) {
        ret = avcodec_receive_frame(decoder->context, decoder->decoded_frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            return;
        } else if (ret < 0) {
            fprintf(stderr, "Error during decoding\n");
            exit(1);
        }
        data_size = av_get_bytes_per_sample(decoder->context->sample_fmt);
        if (data_size < 0) {
            /* This should not occur, checking just for paranoia */
            fprintf(stderr, "Failed to calculate data size\n");
            exit(1);
        }

        for (i = 0; i < decoder->decoded_frame->nb_samples; i++)
            for (ch = 0; ch < decoder->context->channels; ch++)
                fwrite(decoder->decoded_frame->data[ch] + data_size*i, 1, data_size, outfile);
    }

    return;
}

size_t process_one_read(Decoder *ffmpeg_decoder, FILE *infile, FILE *outfile, size_t data_size) {
    int ret;
    int len;
    if (!(ffmpeg_decoder->decoded_frame)) {
        if (!(ffmpeg_decoder->decoded_frame = av_frame_alloc())) {
            fprintf(stderr, "Could not allocate audio frame\n");
            exit(1);
        }
    }
    ret = av_parser_parse2(ffmpeg_decoder->parser,
                           ffmpeg_decoder->context,
                           &(ffmpeg_decoder->packet->data),
                           &(ffmpeg_decoder->packet->size),
                           ffmpeg_decoder->data, data_size,
                           AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
    if (ret < 0) {
        fprintf(stderr, "Error while parsing\n");
        exit(1);
    }
    ffmpeg_decoder->data += ret;
    data_size -= ret;

    if (ffmpeg_decoder->packet->size) {
        decode(ffmpeg_decoder, outfile);
    }

    if (data_size < AUDIO_REFILL_THRESH) {
        memmove(ffmpeg_decoder->inbuf, ffmpeg_decoder->data, data_size);
        ffmpeg_decoder->data = ffmpeg_decoder->inbuf;
        len = fread(ffmpeg_decoder->data + data_size, 1, AUDIO_INBUF_SIZE - data_size, infile);
        if (len > 0)
            data_size += len;
    }

    return data_size;
}
