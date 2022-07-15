#pragma once

#include "consts.hpp"
#define GLEW_STATIC
#include <GL/glew.h>

#include <GLFW/glfw3.h>

#include <fftw3.h>

#define CELL_COUNT WIDTH*HEIGHT

struct Cell {
    float velocity_x;
    float velocity_y;
    float velocity_next_x;
    float velocity_next_y;
    float density;
    float density_next;
    float divergence_x;
    float divergence_y;
    float p_value_x;
    float type;
    //TODO(amatej): there has to be even number of values, I guess for alignments?
    //TODO(amatej): fix the cluster-sock of so many Cell declarations in shaders
};


struct Fluid {
    Cell *cells;
    GLuint diffusion_step_program;
    GLuint diffusiion_finish_program;
    GLuint advection_program;
    GLuint divergence_program;
    GLuint helmholtz_step_program;
    GLuint helmholtz_finish_program;
    GLuint agents_vbo;
    int tex_order;
};

void initialize_fluid(void * mode_data, int count);
void finalize_fluid(void * mode_data);
void run_fluid(void * mode_data, fftw_complex * out_complex_l, float dt,
               int tx0, int tx1, int tx2, int tx3, int tx4, int tx5);
