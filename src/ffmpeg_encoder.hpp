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
#include "ffmpeg_decoder.hpp"

#define AUDIO_INBUF_SIZE 20480
#define AUDIO_REFILL_THRESH 4096

#ifdef av_err2str
#undef av_err2str
#include <string>
av_always_inline std::string av_err2string(int errnum) {
    char str[AV_ERROR_MAX_STRING_SIZE];
    return av_make_error_string(str, AV_ERROR_MAX_STRING_SIZE, errnum);
}
#define av_err2str(err) av_err2string(err).c_str()
#endif  // av_err2str

#ifdef av_ts2str
#undef av_ts2str
#include <string>
av_always_inline std::string av_ts2string(int64_t ts) {
    char buf[AV_TS_MAX_STRING_SIZE];
    if (ts == AV_NOPTS_VALUE) snprintf(buf, AV_TS_MAX_STRING_SIZE, "NOPTS");
    else                      snprintf(buf, AV_TS_MAX_STRING_SIZE, "%" PRId64, ts);
    return buf;
}
#define av_ts2str(ts) av_ts2string(ts).c_str()
#endif  // av_ts2str

#ifdef av_ts2timestr
#undef av_ts2timestr
#include <string>
av_always_inline std::string av_ts2timestring(int64_t ts, AVRational *tb) {
    char buf[AV_TS_MAX_STRING_SIZE];
    if (ts == AV_NOPTS_VALUE) snprintf(buf, AV_TS_MAX_STRING_SIZE, "NOPTS");
    else                      snprintf(buf, AV_TS_MAX_STRING_SIZE, "%.6g", av_q2d(*tb) * ts);
    return buf;
}
#define av_ts2timestr(ts, tb) av_ts2timestring(ts, tb).c_str()
#endif  // av_ts2timestr

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

    struct SwsContext *sws_ctx;
    struct SwrContext *swr_ctx;
} OutputStream;

typedef struct ffmpeg_encoder {
    OutputStream video_st;
    OutputStream audio_st;
    AVFormatContext *output_context;

    const AVCodec *video_codec;
    const AVCodec *audio_codec;
} Encoder;

Encoder *encoder_new(const char *filename);
void encoder_free(Encoder *encoder);

int write_frame(AVFormatContext *fmt_ctx, AVCodecContext *c, AVStream *st, AVFrame *frame, AVPacket *pkt);
int write_audio_frame(Decoder *decoder, AVFormatContext *oc, OutputStream *ost);
