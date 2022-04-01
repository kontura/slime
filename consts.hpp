
#pragma once

//#define WIDTH 2560
//#define HEIGHT 1440
//#define WIDTH 1920
//#define HEIGHT 1080
#define WIDTH 1280
#define HEIGHT 720
//#define WIDTH 480
//#define HEIGHT 360
#define AGENTS_COUNT WIDTH

#define OUTPUT_AUDIO_SAMPLE_RATE 44100
#define FFTW_SAMPLES 1024
// * 2 since we have two channels
#define FFTW_BUFFER_SIZE FFTW_SAMPLES * 2
// *2 for 16bit samples
#define FFTW_BUFFER_BYTES FFTW_BUFFER_SIZE*2

