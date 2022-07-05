#pragma once

#include "consts.hpp"
#define GLEW_STATIC
#include <GL/glew.h>

#include <GLFW/glfw3.h>

#include <fftw3.h>

struct Spectrum {
    float *spectrum_data;
    GLuint spectrum_program;
    GLuint evaporate_program;
    GLuint spectrum_vbo;
    int tex_order;
};

void initialize_spectrum(void * mode_data, int count);
void finalize_spectrum(void * mode_data);
void run_spectrum(void * mode_data, fftw_complex * out_complex_l, float dt,
               int tx0, int tx1, int tx2, int tx3, int tx4, int tx5);
