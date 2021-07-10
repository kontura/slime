#include "pulseAudio.hpp"

#include <cstdio>
#include <cstdlib>

#define CHANNELS_COUNT 2

PulseAudioContext
initializeSimplePulseAudio() {
    pa_simple *s;
    pa_sample_spec ss;

    ss.format = PA_SAMPLE_FLOAT32NE;
    ss.channels = CHANNELS_COUNT;
    ss.rate = 48000;

    pa_buffer_attr battrs;
    battrs.fragsize = (BUFFER_SIZE*CHANNELS_COUNT*sizeof(float)) / 2;
    battrs.maxlength = (BUFFER_SIZE*CHANNELS_COUNT*sizeof(float));

    int err;
    char * sink_name = NULL; //default
    s = pa_simple_new(NULL, "Slime", PA_STREAM_RECORD, sink_name,
                      "Slime input", &ss, NULL, &battrs, &err);

    if (s == NULL) {
        fprintf(stderr, "Failed to initialize PulseAudio: %s\n", pa_strerror(err));
        exit(-12);
    }

    return {s};
}

void destroySimplePulseAudio(PulseAudioContext ctx) {
    pa_simple_free(ctx.s);
}

void readNextPCMFromSimplePulseAudio(PulseAudioContext ctx, float *buffer_left, float *buffer_right) {
    int err;

    //the channels are interlaced in the buffer
    float buff[BUFFER_SIZE*CHANNELS_COUNT];
    if (pa_simple_read(ctx.s, buff, BUFFER_SIZE*CHANNELS_COUNT*sizeof(float), &err) < 0) {
        fprintf(stderr, "Failed to read from PulseAudio: %s\n", pa_strerror(err));
        exit(-12);
    }

    for (int i=0; i<BUFFER_SIZE; i++) {
        buffer_left[i] = buff[i*CHANNELS_COUNT + 0];
        buffer_right[i] = buff[i*CHANNELS_COUNT + 1];
    }
}

