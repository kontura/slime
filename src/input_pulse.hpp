// header file for pulse, part of cava.

#pragma once

#include <pulse/error.h>
#include <pulse/pulseaudio.h>
#include <pulse/simple.h>
#include "consts.hpp"

struct PulseAudioContext {
    pa_simple * s;
    char * source;
    int16_t buf[FFTW_SAMPLES * 2];
};

void *input_pulse(void *data);
void initialize_pulse(void *userdata);
