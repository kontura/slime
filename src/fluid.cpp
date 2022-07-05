#include "consts.hpp"
#include "fluid.hpp"
#include <cstdlib>
#include "util.hpp"
#include <string.h>
#include <string>
#include <cmath>

#define MOVE_SPEED 20.0
#define TURN_SPEED 5.0

#define SENSOR_ANGLE_SPACING 1
#define SENSOR_OFFSET_DST 5
#define SENSOR_SIZE 2
#define EVAPORATE_SPEED 0.20
#define DIFFUSE_SPEED 33.2

static Cell * create_cells(int count) {
    Cell * cell_data = (Cell*) malloc(count*sizeof(Cell));

    for(int i = 0;i<count;++i) {
        // initial position
        cell_data[i].velocity_x = (float)WIDTH/2;
        cell_data[i].velocity_y = (float)HEIGHT/2;
        cell_data[i].density = 1.0f;
    }

    return cell_data;
}

void initialize_fluid(void * mode_data, int count) {
    Fluid *fluid = (Fluid *) mode_data;
    fluid->cells = create_cells(count);

    // generate positions_vbos and vaos and general vbo, ibo
    glGenBuffers(1, &(fluid->agents_vbo));
    //ORIGINAL STUFF FOR AGENTS
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, fluid->agents_vbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float)*3*count, fluid->cells, GL_STATIC_DRAW);

    //This is likely for compute shaders
    const GLuint ssbos[] = {fluid->agents_vbo};
    glBindBuffersBase(GL_SHADER_STORAGE_BUFFER, 0, 1, ssbos);

    // create program
    //TODO(amatej): fix the shaders
    fluid->acceleration_program = generateComputeProgram("../src/compute_shader.glsl");

    // setup uniforms
    glUseProgram(fluid->acceleration_program);
    glUniform1i(glGetUniformLocation(fluid->acceleration_program, "width"), WIDTH);
    glUniform1i(glGetUniformLocation(fluid->acceleration_program, "height"), HEIGHT);
    glUniform1f(glGetUniformLocation(fluid->acceleration_program, "agentsCount"), CELLS_COUNT );

    fluid->tex_order = 1;

    return;
}

void run_fluid(void * mode_data, fftw_complex * out_complex_l, float dt,
               int tx0, int tx1, int tx2, int tx3, int tx4, int tx5) {
//    Slime *slime = (Slime *) mode_data;
    Fluid *fluid = (Fluid *) mode_data;
//    float float_max, float_max_type2;
//    double max_low = 0;
//    double max_high = 0;
//    for (int i=0;i<212;i++) {
//        if (max_low < hypot(out_complex_l[i][0], out_complex_l[i][1])) {
//            max_low = hypot(out_complex_l[i][0], out_complex_l[i][1]);
//        }
//        //printf("%d", hypot(out_complex_l[i][0], out_complex_l[i][1]));
//    }
//    for (int i=212;i<(FFTW_SAMPLES+1);i++) {
//        if (max_high < hypot(out_complex_l[i][0], out_complex_l[i][1])) {
//            max_high = hypot(out_complex_l[i][0], out_complex_l[i][1]);
//        }
//        //printf("%d", hypot(out_complex_l[i][0], out_complex_l[i][1]));
//    }
//    float_max = (float)max_low / (float)51200000* 7;
//    float_max_type2 = (float)max_high / (float)5120000* 7;
//
//    glUseProgram(slime->acceleration_program);
//    glUniform1f(glGetUniformLocation(slime->acceleration_program, "moveSpeed"), 0.3f + float_max*MOVE_SPEED);
//    glUniform1f(glGetUniformLocation(slime->acceleration_program, "turnSpeed"), 0.6f - float_max*TURN_SPEED);
//    glUniform1f(glGetUniformLocation(slime->acceleration_program, "moveSpeedType2"), 0.3 + float_max_type2*MOVE_SPEED);
//    glUniform1f(glGetUniformLocation(slime->acceleration_program, "turnSpeedType2"), 0.6f - float_max_type2*TURN_SPEED);
//    glUniform1f(glGetUniformLocation(slime->acceleration_program, "dt"), dt);
//
//    if (slime->tex_order) {
//        glUniform1i(glGetUniformLocation(slime->acceleration_program, "srcTex"), tx1);
//        glUniform1i(glGetUniformLocation(slime->acceleration_program, "destTex"), tx0);
//        glUniform1i(glGetUniformLocation(slime->acceleration_program, "srcTex_type1"), tx3);
//        glUniform1i(glGetUniformLocation(slime->acceleration_program, "destTex_type1"), tx2);
//        glUniform1i(glGetUniformLocation(slime->acceleration_program, "srcTex_type2"), tx5);
//        glUniform1i(glGetUniformLocation(slime->acceleration_program, "destTex_type2"), tx4);
//    } else {
//        glUniform1i(glGetUniformLocation(slime->acceleration_program, "srcTex"), tx0);
//        glUniform1i(glGetUniformLocation(slime->acceleration_program, "destTex"), tx1);
//        glUniform1i(glGetUniformLocation(slime->acceleration_program, "srcTex_type1"), tx2);
//        glUniform1i(glGetUniformLocation(slime->acceleration_program, "destTex_type1"), tx3);
//        glUniform1i(glGetUniformLocation(slime->acceleration_program, "srcTex_type2"), tx4);
//        glUniform1i(glGetUniformLocation(slime->acceleration_program, "destTex_type2"), tx5);
//    }
//    glDispatchCompute(WIDTH/8, HEIGHT/8, 1);
//
//    //TODO(amatej): this might be possible to optimatize into 1 texture and using different
//    //colorls for diffrent agent types
//    glUseProgram(slime->evaporate_program);
//    if (slime->tex_order) {
//        glUniform1i(glGetUniformLocation(slime->evaporate_program, "srcTex"), tx0);
//        glUniform1i(glGetUniformLocation(slime->evaporate_program, "destTex"), tx1);
//        glUniform1i(glGetUniformLocation(slime->evaporate_program, "srcTex_type1"), tx2);
//        glUniform1i(glGetUniformLocation(slime->evaporate_program, "destTex_type1"), tx3);
//        glUniform1i(glGetUniformLocation(slime->evaporate_program, "srcTex_type2"), tx4);
//        glUniform1i(glGetUniformLocation(slime->evaporate_program, "destTex_type2"), tx5);
//    } else {
//        glUniform1i(glGetUniformLocation(slime->evaporate_program, "srcTex"), tx1);
//        glUniform1i(glGetUniformLocation(slime->evaporate_program, "destTex"), tx0);
//        glUniform1i(glGetUniformLocation(slime->evaporate_program, "srcTex_type1"), tx3);
//        glUniform1i(glGetUniformLocation(slime->evaporate_program, "destTex_type1"), tx2);
//        glUniform1i(glGetUniformLocation(slime->evaporate_program, "srcTex_type2"), tx5);
//        glUniform1i(glGetUniformLocation(slime->evaporate_program, "destTex_type2"), tx4);
//    }
//    glUniform1f(glGetUniformLocation(slime->evaporate_program, "dt"), dt);
//    // We use 40 here because it is a common divider of 1080 and 1920 -> eatch pixel is taken care of in our texture
//    // When setting these don't forget about the local_size in the shader it self
//    glDispatchCompute(WIDTH/20, HEIGHT/20, 1);
//
//    if (slime->tex_order) {
//        slime->tex_order = 0;
//    } else {
//        slime->tex_order = 1;
//    }
}

void finalize_fluid(void * mode_data) {
    Fluid *fluid = (Fluid *) mode_data;
    glDeleteProgram(fluid->acceleration_program);
    glDeleteProgram(fluid->evaporate_program);
    glDeleteBuffers(1, &(fluid->agents_vbo));
    free(fluid->cells);
}
