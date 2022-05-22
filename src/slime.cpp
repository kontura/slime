#include "consts.hpp"
#include "slime.hpp"
#include <cstdlib>
#include "util.hpp"
#include <string.h>
#include <string>
#include <cmath>

Agent * create_agents(int count) {
    Agent *AgentsData = (Agent *) malloc(count*4*sizeof(float));
    // randomly place agents
    //double step = (float)WIDTH/(float)AGENTS_COUNT;
    for(int i = 0;i<count;++i) {
        // initial position
        //AgentsData[i].x = (rand()%WIDTH);
        //AgentsData[i].y = (rand()%HEIGHT);
        //AgentsData[i].x = step*(float)i;
        AgentsData[i].x = WIDTH/2;
        AgentsData[i].y = HEIGHT/2;
        AgentsData[i].angle = 0 * 3.14f * 2.0f;
        AgentsData[i].type = 1;
        if (i > count/2) {
            AgentsData[i].type = 100;
        }
        //printf("angle type: %f \n", AgentsData[i].type);
        //printf("positions data: %f x %f\n", positionData[index], positionData[index+1]);
    }

    return AgentsData;
}

void initialize_slime(Slime * slime, int count) {
    slime->agent_data = create_agents(count);

    // generate positions_vbos and vaos and general vbo, ibo
    glGenBuffers(1, &(slime->agents_vbo));
    //ORIGINAL STUFF FOR AGENTS
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, slime->agents_vbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float)*4*count, slime->agent_data, GL_STATIC_DRAW);

    //This is likely for compute shaders
    const GLuint ssbos[] = {slime->agents_vbo};
    glBindBuffersBase(GL_SHADER_STORAGE_BUFFER, 0, 1, ssbos);

    // create program
    slime->acceleration_program = generateComputeProgram("../src/compute_shader.glsl");
    slime->evaporate_program = generateComputeProgram("../src//evaporate_shader.glsl");

    // setup uniforms
    glUseProgram(slime->acceleration_program);
    glUniform1i(glGetUniformLocation(slime->acceleration_program, "width"), WIDTH);
    glUniform1i(glGetUniformLocation(slime->acceleration_program, "height"), HEIGHT);
    glUniform1f(glGetUniformLocation(slime->acceleration_program, "moveSpeed"), MOVE_SPEED);
    glUniform1f(glGetUniformLocation(slime->acceleration_program, "turnSpeed"), TURN_SPEED);
    glUniform1f(glGetUniformLocation(slime->acceleration_program, "agentsCount"), AGENTS_COUNT );

    glUniform1f(glGetUniformLocation(slime->acceleration_program, "sensorAngleSpacing"), SENSOR_ANGLE_SPACING);
    glUniform1f(glGetUniformLocation(slime->acceleration_program, "sensorOffsetDst"), SENSOR_OFFSET_DST);
    glUniform1i(glGetUniformLocation(slime->acceleration_program, "sensorSize"), SENSOR_SIZE);

    glUseProgram(slime->evaporate_program);
    glUniform1f(glGetUniformLocation(slime->evaporate_program, "evaporate_speed"), EVAPORATE_SPEED);
    glUniform1f(glGetUniformLocation(slime->evaporate_program, "diffuse_speed"), DIFFUSE_SPEED);
    glUniform1i(glGetUniformLocation(slime->evaporate_program, "width"), WIDTH);
    glUniform1i(glGetUniformLocation(slime->evaporate_program, "height"), HEIGHT);

    slime->tex_order = 1;

    return;
}

void run_slime(Slime * slime, fftw_complex * out_complex_l, float dt,
               int tx0, int tx1, int tx2, int tx3, int tx4, int tx5) {
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
    float_max_type2 = (float)max_high / (float)5120000* 7;

    glUseProgram(slime->acceleration_program);
    glUniform1f(glGetUniformLocation(slime->acceleration_program, "moveSpeed"), 0.3f + float_max*MOVE_SPEED);
    glUniform1f(glGetUniformLocation(slime->acceleration_program, "turnSpeed"), 0.6f - float_max*TURN_SPEED);
    glUniform1f(glGetUniformLocation(slime->acceleration_program, "moveSpeedType2"), 0.3 + float_max_type2*MOVE_SPEED);
    glUniform1f(glGetUniformLocation(slime->acceleration_program, "turnSpeedType2"), 0.6f - float_max_type2*TURN_SPEED);
    glUniform1f(glGetUniformLocation(slime->acceleration_program, "dt"), dt);

    if (slime->tex_order) {
        glUniform1i(glGetUniformLocation(slime->acceleration_program, "srcTex"), tx1);
        glUniform1i(glGetUniformLocation(slime->acceleration_program, "destTex"), tx0);
        glUniform1i(glGetUniformLocation(slime->acceleration_program, "srcTex_type1"), tx2);
        glUniform1i(glGetUniformLocation(slime->acceleration_program, "destTex_type1"), tx3);
        glUniform1i(glGetUniformLocation(slime->acceleration_program, "srcTex_type2"), tx4);
        glUniform1i(glGetUniformLocation(slime->acceleration_program, "destTex_type2"), tx5);
    } else {
        glUniform1i(glGetUniformLocation(slime->acceleration_program, "srcTex"), tx0);
        glUniform1i(glGetUniformLocation(slime->acceleration_program, "destTex"), tx1);
        glUniform1i(glGetUniformLocation(slime->acceleration_program, "srcTex_type1"), tx3);
        glUniform1i(glGetUniformLocation(slime->acceleration_program, "destTex_type1"), tx2);
        glUniform1i(glGetUniformLocation(slime->acceleration_program, "srcTex_type2"), tx5);
        glUniform1i(glGetUniformLocation(slime->acceleration_program, "destTex_type2"), tx4);
    }
    glDispatchCompute(WIDTH/8, HEIGHT/8, 1);

    //TODO(amatej): this might be possible to optimatize into 1 texture and using different
    //colorls for diffrent agent types
    glUseProgram(slime->evaporate_program);
    if (slime->tex_order) {
        glUniform1i(glGetUniformLocation(slime->evaporate_program, "srcTex"), tx0);
        glUniform1i(glGetUniformLocation(slime->evaporate_program, "destTex"), tx1);
        glUniform1i(glGetUniformLocation(slime->evaporate_program, "srcTex_type1"), tx2);
        glUniform1i(glGetUniformLocation(slime->evaporate_program, "destTex_type1"), tx3);
        glUniform1i(glGetUniformLocation(slime->evaporate_program, "srcTex_type2"), tx4);
        glUniform1i(glGetUniformLocation(slime->evaporate_program, "destTex_type2"), tx5);
    } else {
        glUniform1i(glGetUniformLocation(slime->evaporate_program, "srcTex"), tx1);
        glUniform1i(glGetUniformLocation(slime->evaporate_program, "destTex"), tx0);
        glUniform1i(glGetUniformLocation(slime->evaporate_program, "srcTex_type1"), tx3);
        glUniform1i(glGetUniformLocation(slime->evaporate_program, "destTex_type1"), tx2);
        glUniform1i(glGetUniformLocation(slime->evaporate_program, "srcTex_type2"), tx5);
        glUniform1i(glGetUniformLocation(slime->evaporate_program, "destTex_type2"), tx4);
    }
    glUniform1f(glGetUniformLocation(slime->evaporate_program, "dt"), dt);
    // We use 40 here because it is a common divider of 1080 and 1920 -> eatch pixel is taken care of in our texture
    // When setting these don't forget about the local_size in the shader it self
    glDispatchCompute(WIDTH/20, HEIGHT/20, 1);

    if (slime->tex_order) {
        slime->tex_order = 0;
    } else {
        slime->tex_order = 1;
    }
}

void finalize_slime(Slime * slime) {
    glDeleteProgram(slime->acceleration_program);
    glDeleteProgram(slime->evaporate_program);
    glDeleteBuffers(1, &(slime->agents_vbo));
    free(slime->agent_data);
}
