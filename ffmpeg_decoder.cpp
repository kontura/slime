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

Encoder *encoder_new(const char *filename) {
    Encoder *encoder = (Encoder *) malloc(sizeof(Encoder));
    memset(encoder, 0, sizeof(Encoder));

    avformat_alloc_output_context2(&(encoder->output_context), NULL, NULL, filename);
    if (!encoder->output_context) {
        printf("Could not deduce output format from file extension: using MPEG.\n");
        avformat_alloc_output_context2(&(encoder->output_context), NULL, "mpeg", filename);
    }
    if (!encoder->output_context) {
        fprintf(stderr, "Encoder codec \"mpeg1video\" not found\n");
        exit(1);
    }



    encoder->packet = av_packet_alloc();

    encoder->video_codec = avcodec_find_encoder_by_name("mpeg1video");
    if (!encoder->video_codec) {
        fprintf(stderr, "Encoder codec \"mpeg1video\" not found\n");
        exit(1);
    }

    encoder->context = avcodec_alloc_context3(encoder->video_codec);
    if (!encoder->context) {
        fprintf(stderr, "Could not allocate audio codec context\n");
        exit(1);
    }
    encoder->context->bit_rate = 400000;
    //resolution must be multiple of 2
    encoder->context->width = WIDTH;
    encoder->context->height = HEIGHT;
    /* frames per second */
    encoder->context->time_base = (AVRational){1, 60};
    encoder->context->framerate = (AVRational){60, 1};
 
    /* emit one intra frame every ten frames
     * check frame pict_type before passing frame
     * to encoder, if frame->pict_type is AV_PICTURE_TYPE_I
     * then gop_size is ignored and the output of encoder
     * will always be I frame irrespective to gop_size
     */
    encoder->context->gop_size = 10;
    encoder->context->max_b_frames = 1;
    encoder->context->pix_fmt = AV_PIX_FMT_YUV420P;

    /* open it */
    if (avcodec_open2(encoder->context, encoder->video_codec, NULL) < 0) {
        fprintf(stderr, "Could not open codec\n");
        exit(1);
    }

    if (!(encoder->encoded_frame = av_frame_alloc())) {
        fprintf(stderr, "Could not allocate encoder frame\n");
        exit(1);
    }

    encoder->encoded_frame->format = encoder->context->pix_fmt;
    encoder->encoded_frame->width = encoder->context->width;
    encoder->encoded_frame->height = encoder->context->height;

    if (av_frame_get_buffer(encoder->encoded_frame, 0) < 0) {
        fprintf(stderr, "Could not allocate the video frame data\n");
        exit(1);
    }

    //decoder->inbuf = (uint8_t *) malloc(sizeof(uint8_t) * AUDIO_INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE);
    //decoder->data = decoder->inbuf;

    return encoder;
}

void decoder_free(Decoder *decoder) {
    avcodec_free_context(&(decoder->context));
    av_parser_close(decoder->parser);
    av_frame_free(&(decoder->decoded_frame));
    av_packet_free(&(decoder->packet));
    free(decoder->inbuf);
    free(decoder);
}

void encoder_free(Encoder *encoder) {
    avcodec_free_context(&(encoder->context));
    av_frame_free(&(encoder->encoded_frame));
    av_packet_free(&(encoder->packet));
    //free(decoder->inbuf);
    //free(decoder);
}

size_t decode(Decoder *decoder, FILE *outfile) {
    int i, ch;
    int ret, data_size;
    size_t total_written_samples = 0;

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
            return total_written_samples;
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

        total_written_samples += decoder->decoded_frame->nb_samples;
        for (i = 0; i < decoder->decoded_frame->nb_samples; i++)
            for (ch = 0; ch < decoder->context->channels; ch++)
                fwrite(decoder->decoded_frame->data[ch] + data_size*i, 1, data_size, outfile);
    }

    return total_written_samples;
}

size_t process_one_read(Decoder *ffmpeg_decoder, FILE *infile, FILE *outfile, size_t data_size, size_t *written_samples) {
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
        *written_samples += decode(ffmpeg_decoder, outfile);
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

size_t encode(Encoder *encoder, FILE *outfile) {
    int ret;
    ret = avcodec_send_frame(encoder->context, encoder->encoded_frame);
    if (ret < 0) {
        fprintf(stderr, "Error sending a frame for encoding\n");
        exit(1);
    }

    while (ret >= 0) {
        ret = avcodec_receive_packet(encoder->context, encoder->packet);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return 0;
        else if (ret < 0) {
            fprintf(stderr, "Error during encoding\n");
            exit(1);
        }

        printf("Write packet %3"PRId64" (size=%5d)\n", encoder->packet->pts, encoder->packet->size);
        fwrite(encoder->packet->data, 1, encoder->packet->size, outfile);
        av_packet_unref(encoder->packet);
    }
    return 0;
}
