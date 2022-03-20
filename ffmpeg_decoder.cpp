#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ffmpeg_decoder.hpp"

//#define STREAM_FRAME_RATE 43.06640625
#define STREAM_FRAME_RATE 43
#define STREAM_PIX_FMT    AV_PIX_FMT_YUV420P /* default pix_fmt */

Decoder *decoder_new(const char *infile) {
    Decoder *decoder = (Decoder *) malloc(sizeof(Decoder));
    memset(decoder, 0, sizeof(Decoder));

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

    decoder->infile = fopen(infile, "rb");
    if (!decoder->infile) {
        fprintf(stderr, "Could not open input file: %s\n", infile);
        exit(1);
    }
    decoder->data_size = fread(decoder->inbuf, 1, AUDIO_INBUF_SIZE, decoder->infile);

    return decoder;
}

/* Add an output stream. */
static void add_stream(OutputStream *ost, AVFormatContext *oc,
                       const AVCodec **codec,
                       enum AVCodecID codec_id)
{
    AVCodecContext *c;
    int i;

    /* find the encoder */
    *codec = avcodec_find_encoder(codec_id);
    if (!(*codec)) {
        fprintf(stderr, "Could not find encoder for '%s'\n",
                avcodec_get_name(codec_id));
        exit(1);
    }

    ost->tmp_pkt = av_packet_alloc();
    if (!ost->tmp_pkt) {
        fprintf(stderr, "Could not allocate AVPacket\n");
        exit(1);
    }

    ost->st = avformat_new_stream(oc, NULL);
    if (!ost->st) {
        fprintf(stderr, "Could not allocate stream\n");
        exit(1);
    }
    ost->st->id = oc->nb_streams-1;
    c = avcodec_alloc_context3(*codec);
    if (!c) {
        fprintf(stderr, "Could not alloc an encoding context\n");
        exit(1);
    }
    ost->enc = c;

    switch ((*codec)->type) {
    case AVMEDIA_TYPE_AUDIO:
        c->sample_fmt  = (*codec)->sample_fmts ? (*codec)->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
        c->bit_rate    = 64000;
        c->sample_rate = 44100;
        if ((*codec)->supported_samplerates) {
            c->sample_rate = (*codec)->supported_samplerates[0];
            for (i = 0; (*codec)->supported_samplerates[i]; i++) {
                if ((*codec)->supported_samplerates[i] == 44100)
                    c->sample_rate = 44100;
            }
        }
        c->channels        = av_get_channel_layout_nb_channels(c->channel_layout);
        c->channel_layout = AV_CH_LAYOUT_STEREO;
        if ((*codec)->channel_layouts) {
            c->channel_layout = (*codec)->channel_layouts[0];
            for (i = 0; (*codec)->channel_layouts[i]; i++) {
                if ((*codec)->channel_layouts[i] == AV_CH_LAYOUT_STEREO)
                    c->channel_layout = AV_CH_LAYOUT_STEREO;
            }
        }
        c->channels        = av_get_channel_layout_nb_channels(c->channel_layout);
        ost->st->time_base = (AVRational){ 1, c->sample_rate };
        break;

    case AVMEDIA_TYPE_VIDEO:
        c->codec_id = codec_id;

        c->bit_rate = 8000000;
        /* Resolution must be a multiple of two. */
        c->width    = WIDTH;
        c->height   = HEIGHT;
        /* timebase: This is the fundamental unit of time (in seconds) in terms
         * of which frame timestamps are represented. For fixed-fps content,
         * timebase should be 1/framerate and timestamp increments should be
         * identical to 1. */
        ost->st->time_base = (AVRational){ 1, STREAM_FRAME_RATE };
        c->time_base       = ost->st->time_base;

        c->gop_size      = 12; /* emit one intra frame every twelve frames at most */
        c->pix_fmt       = STREAM_PIX_FMT;
        if (c->codec_id == AV_CODEC_ID_MPEG2VIDEO) {
            /* just for testing, we also add B-frames */
            c->max_b_frames = 2;
        }
        if (c->codec_id == AV_CODEC_ID_MPEG1VIDEO) {
            /* Needed to avoid using macroblocks in which some coeffs overflow.
             * This does not happen with normal video, it just happens here as
             * the motion of the chroma plane does not match the luma plane. */
            c->mb_decision = 2;
        }
        break;

    default:
        break;
    }

    /* Some formats want stream headers to be separate. */
    if (oc->oformat->flags & AVFMT_GLOBALHEADER)
        c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
}

static AVFrame *alloc_picture(enum AVPixelFormat pix_fmt, int width, int height)
{
    AVFrame *picture;
    int ret;

    picture = av_frame_alloc();
    if (!picture)
        return NULL;

    picture->format = pix_fmt;
    picture->width  = width;
    picture->height = height;

    /* allocate the buffers for the frame data */
    ret = av_frame_get_buffer(picture, 0);
    if (ret < 0) {
        fprintf(stderr, "Could not allocate frame data.\n");
        exit(1);
    }

    return picture;
}

static void open_video(const AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg)
{
    int ret;
    AVCodecContext *c = ost->enc;
    AVDictionary *opt = NULL;

    av_dict_copy(&opt, opt_arg, 0);

    /* open the codec */
    ret = avcodec_open2(c, codec, &opt);
    av_dict_free(&opt);
    if (ret < 0) {
        fprintf(stderr, "Could not open video codec:\n");
        exit(1);
    }

    /* allocate and init a re-usable frame */
    ost->frame = alloc_picture(c->pix_fmt, c->width, c->height);
    if (!ost->frame) {
        fprintf(stderr, "Could not allocate video frame\n");
        exit(1);
    }

    /* If the output format is not YUV420P, then a temporary YUV420P
     * picture is needed too. It is then converted to the required
     * output format. */
    ost->tmp_frame = NULL;
    if (c->pix_fmt != AV_PIX_FMT_YUV420P) {
        ost->tmp_frame = alloc_picture(AV_PIX_FMT_YUV420P, c->width, c->height);
        if (!ost->tmp_frame) {
            fprintf(stderr, "Could not allocate temporary picture\n");
            exit(1);
        }
    }

    /* copy the stream parameters to the muxer */
    ret = avcodec_parameters_from_context(ost->st->codecpar, c);
    if (ret < 0) {
        fprintf(stderr, "Could not copy the stream parameters\n");
        exit(1);
    }
}

static AVFrame *alloc_audio_frame(enum AVSampleFormat sample_fmt,
                                  uint64_t channel_layout,
                                  int sample_rate, int nb_samples)
{
    AVFrame *frame = av_frame_alloc();
    int ret;

    if (!frame) {
        fprintf(stderr, "Error allocating an audio frame\n");
        exit(1);
    }

    frame->format = sample_fmt;
    frame->channel_layout = channel_layout;
    frame->sample_rate = sample_rate;
    frame->nb_samples = nb_samples;

    if (nb_samples) {
        ret = av_frame_get_buffer(frame, 0);
        if (ret < 0) {
            fprintf(stderr, "Error allocating an audio buffer\n");
            exit(1);
        }
    }

    return frame;
}

static void open_audio(const AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg)
{
    AVCodecContext *c;
    int nb_samples;
    int ret;
    AVDictionary *opt = NULL;

    c = ost->enc;

    /* open it */
    av_dict_copy(&opt, opt_arg, 0);
    ret = avcodec_open2(c, codec, &opt);
    av_dict_free(&opt);
    if (ret < 0) {
        fprintf(stderr, "Could not open audio codec\n");
        exit(1);
    }

    /* init signal generator */
    ost->t     = 0;
    ost->tincr = 2 * M_PI * 110.0 / c->sample_rate;
    /* increment frequency by 110 Hz per second */
    ost->tincr2 = 2 * M_PI * 110.0 / c->sample_rate / c->sample_rate;

    if (c->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE) {
        nb_samples = 10000;
        printf("variable size\n");
    } else {
        nb_samples = c->frame_size;
        printf("invariable size: %i\n", c->frame_size);
    }

    ost->frame     = alloc_audio_frame(c->sample_fmt, c->channel_layout,
                                       c->sample_rate, nb_samples);
    ost->tmp_frame = alloc_audio_frame(AV_SAMPLE_FMT_S16, c->channel_layout,
                                       c->sample_rate, nb_samples);

    /* copy the stream parameters to the muxer */
    ret = avcodec_parameters_from_context(ost->st->codecpar, c);
    if (ret < 0) {
        fprintf(stderr, "Could not copy the stream parameters\n");
        exit(1);
    }

    /* create resampler context */
    ost->swr_ctx = swr_alloc();
    if (!ost->swr_ctx) {
        fprintf(stderr, "Could not allocate resampler context\n");
        exit(1);
    }

    /* set options */
    av_opt_set_int       (ost->swr_ctx, "in_channel_count",   c->channels,       0);
    av_opt_set_int       (ost->swr_ctx, "in_sample_rate",     c->sample_rate,    0);
    av_opt_set_sample_fmt(ost->swr_ctx, "in_sample_fmt",      AV_SAMPLE_FMT_S16, 0);
    av_opt_set_int       (ost->swr_ctx, "out_channel_count",  c->channels,       0);
    av_opt_set_int       (ost->swr_ctx, "out_sample_rate",    c->sample_rate,    0);
    av_opt_set_sample_fmt(ost->swr_ctx, "out_sample_fmt",     c->sample_fmt,     0);

    /* initialize the resampling context */
    if ((ret = swr_init(ost->swr_ctx)) < 0) {
        fprintf(stderr, "Failed to initialize the resampling context\n");
        exit(1);
    }
}


Encoder *encoder_new(const char *filename) {
    Encoder *encoder = (Encoder *) malloc(sizeof(Encoder));
    memset(encoder, 0, sizeof(Encoder));
    //TODO(amatej): we might add last params AVDictionary with options..
    AVDictionary *opt = NULL;

    avformat_alloc_output_context2(&(encoder->output_context), NULL, NULL, filename);
    if (!encoder->output_context) {
        printf("Could not deduce output format from file extension: using MPEG.\n");
        avformat_alloc_output_context2(&(encoder->output_context), NULL, "mpeg", filename);
    }
    if (!encoder->output_context) {
        fprintf(stderr, "Encoder codec \"mpeg1video\" not found\n");
        exit(1);
    }

    if (encoder->output_context->oformat->video_codec != AV_CODEC_ID_NONE) {
        add_stream(&(encoder->video_st), encoder->output_context, &(encoder->video_codec),
                   encoder->output_context->oformat->video_codec);
    }
    if (encoder->output_context->oformat->audio_codec != AV_CODEC_ID_NONE) {
        add_stream(&(encoder->audio_st), encoder->output_context, &(encoder->audio_codec),
                   encoder->output_context->oformat->audio_codec);
    }

    open_video(encoder->video_codec, &(encoder->video_st), opt);
    open_audio(encoder->audio_codec, &(encoder->audio_st), opt);

    av_dump_format(encoder->output_context, 0, filename, 1);

    /* open the output file, if needed */
    if (!(encoder->output_context->oformat->flags & AVFMT_NOFILE)) {
        int ret = avio_open(&(encoder->output_context->pb), filename, AVIO_FLAG_WRITE);
        if (ret < 0) {
            fprintf(stderr, "Could not open '%s'\n", filename);
            exit(1);
        }
    }

    /* Write the stream header, if any. */
    int ret = avformat_write_header(encoder->output_context, &opt);
    if (ret < 0) {
        fprintf(stderr, "Error occurred when opening output file\n");
        exit(1);
    }

    return encoder;
}

void decoder_free(Decoder *decoder) {
    avcodec_free_context(&(decoder->context));
    av_parser_close(decoder->parser);
    av_frame_free(&(decoder->decoded_frame));
    av_packet_free(&(decoder->packet));
    fclose(decoder->infile);
    free(decoder->inbuf);
    free(decoder);
}

static void close_stream(OutputStream *ost)
{
    avcodec_free_context(&ost->enc);
    av_frame_free(&ost->frame);
    av_frame_free(&ost->tmp_frame);
    av_packet_free(&ost->tmp_pkt);
    sws_freeContext(ost->sws_ctx);
    swr_free(&ost->swr_ctx);
}

void encoder_free(Encoder *encoder) {
    /* Write the trailer, if any. The trailer must be written before you
     * close the CodecContexts open when you wrote the header; otherwise
     * av_write_trailer() may try to use memory that was freed on
     * av_codec_close(). */
    av_write_trailer(encoder->output_context);
    /* Close each codec. */
    close_stream(&(encoder->video_st));
    close_stream(&(encoder->audio_st));

    if (!(encoder->output_context->oformat->flags & AVFMT_NOFILE)) {
        /* Close the output file. */
        avio_closep(&(encoder->output_context->pb));
    }

    /* free the stream */
    avformat_free_context(encoder->output_context);
}

void decode(Decoder *decoder) {
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


        size_t new_samples_bytes_count = decoder->context->channels * decoder->decoded_frame->nb_samples * data_size;
        decoder->samples_buffer = (uint8_t*) realloc(decoder->samples_buffer, decoder->samples_buffer_count + new_samples_bytes_count);

        size_t total = 0;
        for (i = 0; i < decoder->decoded_frame->nb_samples; i++)
            for (ch = 0; ch < decoder->context->channels; ch++) {
                total += data_size;
                memmove(decoder->samples_buffer + decoder->samples_buffer_count,
                        decoder->decoded_frame->data[ch] + data_size*i,
                        data_size);
                decoder->samples_buffer_count += data_size;
            }
        //printf("total: %i x new_samples_count: %lu\n", total, new_samples_bytes_count);
    }

    return;
}

size_t load_audio_samples(Decoder *ffmpeg_decoder) {
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
                           ffmpeg_decoder->data, ffmpeg_decoder->data_size,
                           AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
    if (ret < 0) {
        fprintf(stderr, "Error while parsing\n");
        exit(1);
    }
    ffmpeg_decoder->data += ret;
    ffmpeg_decoder->data_size -= ret;

    if (ffmpeg_decoder->packet->size) {
        decode(ffmpeg_decoder);
    }

    if (ffmpeg_decoder->data_size < AUDIO_REFILL_THRESH) {
        memmove(ffmpeg_decoder->inbuf, ffmpeg_decoder->data, ffmpeg_decoder->data_size);
        ffmpeg_decoder->data = ffmpeg_decoder->inbuf;
        len = fread(ffmpeg_decoder->data + ffmpeg_decoder->data_size, 1, AUDIO_INBUF_SIZE - ffmpeg_decoder->data_size,
                    ffmpeg_decoder->infile);
        if (len > 0)
            ffmpeg_decoder->data_size += len;
    }

    return ffmpeg_decoder->data_size;
}

int write_frame(AVFormatContext *fmt_ctx, AVCodecContext *c,
                       AVStream *st, AVFrame *frame, AVPacket *pkt)
{
    int ret;

    // send the frame to the encoder
    ret = avcodec_send_frame(c, frame);
    if (ret < 0) {
        fprintf(stderr, "Error sending a frame to the encoder\n");
        exit(1);
    }

    while (ret >= 0) {
        ret = avcodec_receive_packet(c, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            break;
        else if (ret < 0) {
            fprintf(stderr, "Error encoding a frame\n");
            exit(1);
        }

        /* rescale output packet timestamp values from codec to stream timebase */
        av_packet_rescale_ts(pkt, c->time_base, st->time_base);
        pkt->stream_index = st->index;

        /* Write the compressed frame to the media file. */
        ret = av_interleaved_write_frame(fmt_ctx, pkt);
        /* pkt is now blank (av_interleaved_write_frame() takes ownership of
         * its contents and resets pkt), so that no unreferencing is necessary.
         * This would be different if one used av_write_frame(). */
        if (ret < 0) {
            fprintf(stderr, "Error while writing output packet\n");
            exit(1);
        }
    }

    return ret == AVERROR_EOF ? 1 : 0;
}

/* Prepare a 16 bit dummy audio frame of 'frame_size' samples and
 * 'nb_channels' channels. */
static AVFrame *get_audio_frame(OutputStream *ost)
{
    AVFrame *frame = ost->tmp_frame;
    int j, i, v;
    int16_t *q = (int16_t*)frame->data[0];
    printf("nb_samples: %i\n", frame->nb_samples);

    for (j = 0; j <frame->nb_samples; j++) {
        v = (int)(sin(ost->t) * 10000);
        for (i = 0; i < ost->enc->channels; i++)
            *q++ = v;
        ost->t     += ost->tincr;
        ost->tincr += ost->tincr2;
    }

    frame->pts = ost->next_pts;
    ost->next_pts  += frame->nb_samples;

    return frame;
}

/*
 * encode one audio frame and send it to the muxer
 * return 1 when encoding is finished, 0 otherwise
 */
int write_audio_frame(Decoder *decoder, AVFormatContext *oc, OutputStream *ost)
{
    AVCodecContext *c;
    int ret;
    int dst_nb_samples;

    c = ost->enc;

    //AVFrame *frame = ost->tmp_frame;
    //memcpy(frame->data, decoder->samples_buffer, CAVA_BYTES_READ_COUNT);
    //frame->pts = ost->next_pts;
    //ost->next_pts  += frame->nb_samples;
    //frame = get_audio_frame(ost);


    AVFrame *frame = ost->tmp_frame;
    int j, i, v;
    int16_t *q = (int16_t*)frame->data[0];

    memcpy(q, decoder->samples_buffer, CAVA_BYTES_READ_COUNT);

    frame->pts = ost->next_pts;
    ost->next_pts  += frame->nb_samples;




    if (frame) {
        /* convert samples from native format to destination codec format, using the resampler */
        /* compute destination number of samples */
        dst_nb_samples = av_rescale_rnd(swr_get_delay(ost->swr_ctx, c->sample_rate) + frame->nb_samples,
                                        c->sample_rate, c->sample_rate, AV_ROUND_UP);
        av_assert0(dst_nb_samples == frame->nb_samples);

        /* when we pass a frame to the encoder, it may keep a reference to it
         * internally;
         * make sure we do not overwrite it here
         */
        ret = av_frame_make_writable(ost->frame);
        if (ret < 0)
            exit(1);

        /* convert to destination format */
        ret = swr_convert(ost->swr_ctx,
                          ost->frame->data, dst_nb_samples,
                          (const uint8_t **)frame->data, frame->nb_samples);
        if (ret < 0) {
            fprintf(stderr, "Error while converting\n");
            exit(1);
        }
        frame = ost->frame;

        frame->pts = av_rescale_q(ost->samples_count, (AVRational){1, c->sample_rate}, c->time_base);
        ost->samples_count += dst_nb_samples;
    }

    return write_frame(oc, c, ost->st, frame, ost->tmp_pkt);
}

//void make_example_video(Encoder *encoder) {
//    int encode_video = 1, encode_audio = 1;
//
//    while (encode_video || encode_audio) {
//        /* select the stream to encode */
//        if (encode_video && (!encode_audio ||
//                             av_compare_ts(encoder->video_st.next_pts,
//                                           encoder->video_st.enc->time_base,
//                                           encoder->audio_st.next_pts,
//                                           encoder->audio_st.enc->time_base) <= 0)) {
//            encode_video = !write_video_frame(encoder->output_context, &(encoder->video_st));
//        } else {
//            encode_audio = !write_audio_frame(encoder->output_context, &(encoder->audio_st));
//        }
//    }
//
//
//}
