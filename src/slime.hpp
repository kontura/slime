#pragma once

#include "consts.hpp"
#define GLEW_STATIC
#include <GL/glew.h>

#include <GLFW/glfw3.h>

#include <fftw3.h>

#define AGENTS_COUNT WIDTH

struct Agent {
    float x;
    float y;
    float angle;
    float type;
};

struct Slime {
    Agent *agent_data;
    GLuint acceleration_program;
    GLuint evaporate_program;
    GLuint agents_vbo;
    int tex_order;
};

Agent * create_agents(int count);

void initialize_slime(Slime * slime, int count);
void finalize_slime(Slime * slime);
void run_slime(Slime * slime, fftw_complex * out_complex_l, float dt,
               int tx0, int tx1, int tx2, int tx3, int tx4, int tx5);
