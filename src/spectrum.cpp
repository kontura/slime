#include "consts.hpp"
#include "spectrum.hpp"
#include <cstdlib>
#include "util.hpp"
#include <string.h>
#include <string>
#include <cmath>

void initialize_spectrum(Spectrum * spectrum, int count) {
    spectrum->spectrum_data = (float *) malloc(count*sizeof(float));

    // generate positions_vbos and vaos and general vbo, ibo
    glGenBuffers(1, &(spectrum->spectrum_vbo));
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, spectrum->spectrum_vbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float)*count, spectrum->spectrum_data, GL_STATIC_DRAW);

    //This is likely for compute shaders
    const GLuint ssbos[] = {spectrum->spectrum_vbo};
    glBindBuffersBase(GL_SHADER_STORAGE_BUFFER, 0, 1, ssbos);

    // create program
    spectrum->spectrum_program = generateComputeProgram("../src/spectrum_shader.glsl");
    spectrum->evaporate_program = generateComputeProgram("../src//evaporate_shader.glsl");

    glUseProgram(spectrum->spectrum_program);
    glUniform1i(glGetUniformLocation(spectrum->spectrum_program, "width"), WIDTH);
    glUniform1i(glGetUniformLocation(spectrum->spectrum_program, "height"), HEIGHT);
    glUniform1f(glGetUniformLocation(spectrum->spectrum_program, "count"), count );

    glUseProgram(spectrum->evaporate_program);
    glUniform1f(glGetUniformLocation(spectrum->evaporate_program, "evaporate_speed"), EVAPORATE_SPEED);
    glUniform1f(glGetUniformLocation(spectrum->evaporate_program, "diffuse_speed"), DIFFUSE_SPEED);
    glUniform1i(glGetUniformLocation(spectrum->evaporate_program, "width"), WIDTH);
    glUniform1i(glGetUniformLocation(spectrum->evaporate_program, "height"), HEIGHT);

    spectrum->tex_order = 1;
}

void finalize_spectrum(Spectrum * spectrum) {
    glDeleteProgram(spectrum->spectrum_program);
    glDeleteProgram(spectrum->evaporate_program);
    glDeleteBuffers(1, &(spectrum->spectrum_vbo));
    free(spectrum->spectrum_data);
}

void run_spectrum(Spectrum * spectrum, fftw_complex * out_complex_l, float dt,
                  int tx0, int tx1, int tx2, int tx3, int tx4, int tx5) {
    for (int i=0;i<(FFTW_SAMPLES+1);i++) {
        spectrum->spectrum_data[i] = hypot(out_complex_l[i][0], out_complex_l[i][1]) / 500;
    }
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, spectrum->spectrum_vbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float)*1024, spectrum->spectrum_data, GL_STATIC_DRAW);

    //This is likely for compute shaders
    const GLuint ssbos[] = {spectrum->spectrum_vbo};
    glBindBuffersBase(GL_SHADER_STORAGE_BUFFER, 0, 1, ssbos);

    glUseProgram(spectrum->spectrum_program);
    glUniform1f(glGetUniformLocation(spectrum->spectrum_program, "dt"), dt);

    if (spectrum->tex_order) {
        glUniform1i(glGetUniformLocation(spectrum->spectrum_program, "srcTex"), tx1);
        glUniform1i(glGetUniformLocation(spectrum->spectrum_program, "destTex"), tx0);
        glUniform1i(glGetUniformLocation(spectrum->spectrum_program, "srcTex_type1"), tx2);
        glUniform1i(glGetUniformLocation(spectrum->spectrum_program, "destTex_type1"), tx3);
        glUniform1i(glGetUniformLocation(spectrum->spectrum_program, "srcTex_type2"), tx4);
        glUniform1i(glGetUniformLocation(spectrum->spectrum_program, "destTex_type2"), tx5);
    } else {
        glUniform1i(glGetUniformLocation(spectrum->spectrum_program, "srcTex"), tx0);
        glUniform1i(glGetUniformLocation(spectrum->spectrum_program, "destTex"), tx1);
        glUniform1i(glGetUniformLocation(spectrum->spectrum_program, "srcTex_type1"), tx3);
        glUniform1i(glGetUniformLocation(spectrum->spectrum_program, "destTex_type1"), tx2);
        glUniform1i(glGetUniformLocation(spectrum->spectrum_program, "srcTex_type2"), tx5);
        glUniform1i(glGetUniformLocation(spectrum->spectrum_program, "destTex_type2"), tx4);
    }
    glDispatchCompute(160, 1, 1);

    glUseProgram(spectrum->evaporate_program);
    if (spectrum->tex_order) {
        glUniform1i(glGetUniformLocation(spectrum->evaporate_program, "srcTex"), tx0);
        glUniform1i(glGetUniformLocation(spectrum->evaporate_program, "destTex"), tx1);
        glUniform1i(glGetUniformLocation(spectrum->evaporate_program, "srcTex_type1"), tx2);
        glUniform1i(glGetUniformLocation(spectrum->evaporate_program, "destTex_type1"), tx3);
        glUniform1i(glGetUniformLocation(spectrum->evaporate_program, "srcTex_type2"), tx4);
        glUniform1i(glGetUniformLocation(spectrum->evaporate_program, "destTex_type2"), tx5);
    } else {
        glUniform1i(glGetUniformLocation(spectrum->evaporate_program, "srcTex"), tx1);
        glUniform1i(glGetUniformLocation(spectrum->evaporate_program, "destTex"), tx0);
        glUniform1i(glGetUniformLocation(spectrum->evaporate_program, "srcTex_type1"), tx3);
        glUniform1i(glGetUniformLocation(spectrum->evaporate_program, "destTex_type1"), tx2);
        glUniform1i(glGetUniformLocation(spectrum->evaporate_program, "srcTex_type2"), tx5);
        glUniform1i(glGetUniformLocation(spectrum->evaporate_program, "destTex_type2"), tx4);
    }
    glUniform1f(glGetUniformLocation(spectrum->evaporate_program, "dt"), dt);
    // We use 40 here because it is a common divider of 1080 and 1920 -> eatch pixel is taken care of in our texture
    // When setting these don't forget about the local_size in the shader it self
    glDispatchCompute(WIDTH/20, HEIGHT/20, 1);

    if (spectrum->tex_order) {
        spectrum->tex_order = 0;
    } else {
        spectrum->tex_order = 1;
    }
}
