#pragma once

#include "consts.hpp"
#define GLEW_STATIC
#include <GL/glew.h>

#include <GLFW/glfw3.h>

#include <fftw3.h>

#define CELLS_COUNT WIDTH*HEIGHT

struct Cell {
    float velocity_x;
    float velocity_y;
    float density;
};


struct Fluid {
    Cell *cells;
    GLuint acceleration_program;
    GLuint evaporate_program;
    GLuint agents_vbo;
    int tex_order;
};

void initialize_fluid(void * mode_data, int count);
void finalize_fluid(void * mode_data);
void run_fluid(void * mode_data, fftw_complex * out_complex_l, float dt,
               int tx0, int tx1, int tx2, int tx3, int tx4, int tx5);
