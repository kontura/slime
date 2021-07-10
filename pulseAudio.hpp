#pragma once

#include <pulse/simple.h>
#include <pulse/error.h>

#define BUFFER_SIZE 512

struct PulseAudioContext {
    pa_simple *s;
};

PulseAudioContext initializeSimplePulseAudio();
void destroySimplePulseAudio(PulseAudioContext ctx);
void readNextPCMFromSimplePulseAudio(PulseAudioContext ctx, float *buffer_left, float *buffer_right);

