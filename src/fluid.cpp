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

#define PATH_TO_CELL_DEFINITION "../src/shaders/fluid/cell.glsl"

static Cell * create_cells(int count) {
    Cell * cell_data = (Cell*) malloc(count*sizeof(Cell));

    for(int i = 0;i<count;++i) {
        // initial position
        if (i > count/8*3 && i < count/8*4) {
            cell_data[i].velocity_x = 20.0f;
            cell_data[i].velocity_y = -200.0f;
            cell_data[i].density = 1.0f;
            cell_data[i].type = 1.0f;
        } else if (i > count/8*4 && i < count/8*5) {
            cell_data[i].velocity_x = -200.0f;
            cell_data[i].velocity_y = 90.0f;
            cell_data[i].density = 1.0f;
            cell_data[i].type = 0.0f;
        } else {
            cell_data[i].velocity_x = 0.0f;
            cell_data[i].velocity_y = 0.0f;
            cell_data[i].density = 0.0f;
        }
        cell_data[i].density_next = 0.0f;
        cell_data[i].velocity_next_x = 0.0f;
        cell_data[i].velocity_next_y = 0.0f;
        cell_data[i].divergence_x = 0.0f;
        cell_data[i].divergence_y = 0.0f;
        cell_data[i].p_value_x = 0.0f;
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
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Cell)*count, fluid->cells, GL_STATIC_DRAW);

    //This is likely for compute shaders
    const GLuint ssbos[] = {fluid->agents_vbo};
    glBindBuffersBase(GL_SHADER_STORAGE_BUFFER, 0, 1, ssbos);

    // create program
    fluid->diffusion_step_program = generateComputeProgram({PATH_TO_CELL_DEFINITION, "../src/shaders/fluid/diffusion_step.glsl"});

    // setup uniforms
    glUseProgram(fluid->diffusion_step_program);
    glUniform1i(glGetUniformLocation(fluid->diffusion_step_program, "width"), WIDTH);
    glUniform1i(glGetUniformLocation(fluid->diffusion_step_program, "height"), HEIGHT);

    fluid->diffusiion_finish_program = generateComputeProgram({PATH_TO_CELL_DEFINITION, "../src/shaders/fluid/diffusion_finish.glsl"});
    glUseProgram(fluid->diffusiion_finish_program);
    glUniform1i(glGetUniformLocation(fluid->diffusiion_finish_program, "width"), WIDTH);

    fluid->advection_program = generateComputeProgram({PATH_TO_CELL_DEFINITION, "../src/shaders/fluid/advection.glsl"});
    glUseProgram(fluid->advection_program);
    glUniform1i(glGetUniformLocation(fluid->advection_program, "width"), WIDTH);
    glUniform1i(glGetUniformLocation(fluid->advection_program, "height"), HEIGHT);

    fluid->divergence_program = generateComputeProgram({PATH_TO_CELL_DEFINITION, "../src/shaders/fluid/divergence.glsl"});
    glUseProgram(fluid->divergence_program);
    glUniform1i(glGetUniformLocation(fluid->divergence_program, "width"), WIDTH);
    glUniform1i(glGetUniformLocation(fluid->divergence_program, "height"), HEIGHT);

    fluid->helmholtz_step_program = generateComputeProgram({PATH_TO_CELL_DEFINITION, "../src/shaders/fluid/helmholtz_step.glsl"});
    glUseProgram(fluid->helmholtz_step_program);
    glUniform1i(glGetUniformLocation(fluid->helmholtz_step_program, "width"), WIDTH);
    glUniform1i(glGetUniformLocation(fluid->helmholtz_step_program, "height"), HEIGHT);

    fluid->helmholtz_finish_program = generateComputeProgram({PATH_TO_CELL_DEFINITION, "../src/shaders/fluid/helmholtz_finish.glsl"});
    glUseProgram(fluid->helmholtz_finish_program);
    glUniform1i(glGetUniformLocation(fluid->helmholtz_finish_program, "width"), WIDTH);
    glUniform1i(glGetUniformLocation(fluid->helmholtz_finish_program, "height"), HEIGHT);

    fluid->tex_order = 1;

    return;
}

void run_fluid(void * mode_data, fftw_complex * out_complex_l, float dt,
               int tx0, int tx1, int tx2, int tx3, int tx4, int tx5) {
    Fluid *fluid = (Fluid *) mode_data;
    float float_max, float_max_type2;
    double max_low = 0;
    double max_high = 0;
    for (int i=0;i<212;i++) {
        if (max_low < hypot(out_complex_l[i][0], out_complex_l[i][1])) {
            max_low = hypot(out_complex_l[i][0], out_complex_l[i][1]);
        }
        //printf("%d", hypot(out_complex_l[i][0], out_complex_l[i][1]));
    }
    for (int i=212;i<(FFTW_SAMPLES+1);i++) {
        if (max_high < hypot(out_complex_l[i][0], out_complex_l[i][1])) {
            max_high = hypot(out_complex_l[i][0], out_complex_l[i][1]);
        }
        //printf("%d", hypot(out_complex_l[i][0], out_complex_l[i][1]));
    }
    float_max = (float)max_low / (float)51200000* 7;
    float_max_type2 = (float)max_high / (float)512000* 7;

    glUseProgram(fluid->diffusion_step_program);
    glUniform1f(glGetUniformLocation(fluid->diffusion_step_program, "dt"), dt);

    if (fluid->tex_order) {
        glUniform1i(glGetUniformLocation(fluid->diffusion_step_program, "srcTex"), tx1);
        glUniform1i(glGetUniformLocation(fluid->diffusion_step_program, "destTex"), tx0);
    } else {
        glUniform1i(glGetUniformLocation(fluid->diffusion_step_program, "srcTex"), tx0);
        glUniform1i(glGetUniformLocation(fluid->diffusion_step_program, "destTex"), tx1);
    }
    glDispatchCompute(WIDTH/20, HEIGHT/20, 8);

    glUseProgram(fluid->diffusiion_finish_program);
    glDispatchCompute(WIDTH/20, HEIGHT/20, 1);

    glUseProgram(fluid->advection_program);
    glUniform1f(glGetUniformLocation(fluid->advection_program, "dt"), dt);
    printf("float_max: %f\n", float_max_type2);
    glUniform1f(glGetUniformLocation(fluid->advection_program, "speed"), float_max_type2);
    glDispatchCompute(WIDTH/20, HEIGHT/20, 1);

    glUseProgram(fluid->divergence_program);
    glDispatchCompute(WIDTH/20, HEIGHT/20, 1);

    glUseProgram(fluid->helmholtz_step_program);
    glDispatchCompute(WIDTH/20, HEIGHT/20, 8);

    glUseProgram(fluid->helmholtz_finish_program);
    glDispatchCompute(WIDTH/20, HEIGHT/20, 1);

    if (fluid->tex_order) {
        fluid->tex_order = 0;
    } else {
        fluid->tex_order = 1;
    }
}

void finalize_fluid(void * mode_data) {
    Fluid *fluid = (Fluid *) mode_data;
    glDeleteProgram(fluid->diffusion_step_program);
    glDeleteProgram(fluid->diffusiion_finish_program);
    glDeleteBuffers(1, &(fluid->agents_vbo));
    free(fluid->cells);
}
