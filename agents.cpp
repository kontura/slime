#define GLEW_STATIC
#include <GL/glew.h>

#include <GLFW/glfw3.h>

#include <unistd.h>
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <cmath>
#include <fstream>
#include <fcntl.h>

#include "pulseAudio.hpp"
#include "cava.hpp"

#define AGENTS_COUNT 1024 * 1024
#define WIDTH 1280
#define HEIGHT 720

#define EVAPORATE_SPEED 0.20
#define DIFFUSE_SPEED 33.2
#define MOVE_SPEED 20.0
#define TURN_SPEED 5.0

#define SENSOR_ANGLE_SPACING 1
#define SENSOR_OFFSET_DST 5
#define SENSOR_SIZE 2


std::string read_file(std::string path) {

  std::ifstream ifs(path);
  std::string content( (std::istreambuf_iterator<char>(ifs) ),
                       (std::istreambuf_iterator<char>()    ) );

  return content;
}

// helper to check and display for shader compiler errors
bool check_shader_compile_status(GLuint obj) {
    GLint status;
    glGetShaderiv(obj, GL_COMPILE_STATUS, &status);
    if(status == GL_FALSE) {
        GLint length;
        glGetShaderiv(obj, GL_INFO_LOG_LENGTH, &length);
        std::vector<char> log(length);
        glGetShaderInfoLog(obj, length, &length, &log[0]);
        std::cerr << &log[0];
        return false;
    }
    return true;
}

// helper to check and display for shader linker error
bool check_program_link_status(GLuint obj) {
    GLint status;
    glGetProgramiv(obj, GL_LINK_STATUS, &status);
    if(status == GL_FALSE) {
        GLint length;
        glGetProgramiv(obj, GL_INFO_LOG_LENGTH, &length);
        std::vector<char> log(length);
        glGetProgramInfoLog(obj, length, &length, &log[0]);
        std::cerr << &log[0];
        return false;
    }
    return true;
}

void checkOpenGLErrors(const char *location) {
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        fprintf(stderr, "OpenGL error from: %s: %s (%d)\n", location, gluErrorString(err), err);
        exit(-1);
    }
}

GLFWwindow *initializeOpenGl() {
    if(glfwInit() == GL_FALSE) {
        std::cerr << "failed to init GLFW" << std::endl;
        exit(11);
    }

    // select opengl version
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    // create a window
    GLFWwindow *window;
    if((window = glfwCreateWindow(WIDTH, HEIGHT, "handmade_slime", 0, 0)) == 0) {
        std::cerr << "failed to open window" << std::endl;
        glfwTerminate();
        exit(11);
    }

    glfwMakeContextCurrent(window);

    if(glewInit()) {
        std::cerr << "failed to init GL3W" << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        exit(11);
    }

    return window;
}

GLuint generateRenderProgram() {
    GLuint program = glCreateProgram();

    // shader source code
    std::string vertex_source =
        "#version 430\n"
        "layout(location = 0) in vec4 vposition;\n"
        "layout(location = 1) in vec2 vtexcoord;\n"
        "out vec2 ftexcoord;\n"
        "void main() {\n"
        "   ftexcoord = vtexcoord;\n"
        "   gl_Position = vposition;\n"
        "}\n";

    std::string fragment_source =
        "#version 430\n"
        "uniform sampler2D tex;\n" // texture uniform
        "in vec2 ftexcoord;\n"
        "layout(location = 0) out vec4 FragColor;\n"
        "void main() {\n"
        "   FragColor = texture(tex, ftexcoord);\n"
        "}\n";

    // create and compiler vertex shader

    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    int length = vertex_source.size();
    const char *source = vertex_source.c_str();
    glShaderSource(vertex_shader, 1, &source, &length);
    glCompileShader(vertex_shader);
    if(!check_shader_compile_status(vertex_shader)) {
        return 0;
    }

    // create and compiler fragment shader
    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    length = fragment_source.size();
    source = fragment_source.c_str();
    glShaderSource(fragment_shader, 1, &source, &length);
    glCompileShader(fragment_shader);
    if(!check_shader_compile_status(fragment_shader)) {
        return 0;
    }

    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);

    glLinkProgram(program);
    glUseProgram(program);

    // link the program and check for errors
    check_program_link_status(program);

    // data for a fullscreen quad (this time with texture coords)
    GLfloat vertexData[] = {
    //  X     Y     Z           U     V
       1.0f, 1.0f, 0.0f,       1.0f, 1.0f, // vertex 0
      -1.0f, 1.0f, 0.0f,       0.0f, 1.0f, // vertex 1
       1.0f,-1.0f, 0.0f,       1.0f, 0.0f, // vertex 2
      -1.0f,-1.0f, 0.0f,       0.0f, 0.0f, // vertex 3
    }; // 4 vertices with 5 components (floats) each

    GLuint indexData[] = {
        0,1,2, // first triangle
        2,1,3, // second triangle
    };

    GLuint vao, vbo, ibo;

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ibo);

    // generate and bind the vertex buffer object
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    // fill with data
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*4*5, vertexData, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5*sizeof(GLfloat), (char*)0 + 0*sizeof(GLfloat));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5*sizeof(GLfloat), (char*)0 + 3*sizeof(GLfloat));

    // generate and bind the index buffer object
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    // fill with data
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint)*2*3, indexData, GL_STATIC_DRAW);

    glDeleteShader(fragment_shader);
    glDeleteShader(vertex_shader);

    checkOpenGLErrors("generateRenderProgram");

    return program;
}

GLuint generateTexture(int texture_slot) {
    GLuint texture_handle;
    glGenTextures(1, &texture_handle);
    glActiveTexture(GL_TEXTURE0 + texture_slot);
    glBindTexture(GL_TEXTURE_2D, texture_handle);

    // set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // set texture content
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, WIDTH, HEIGHT, 0, GL_RGBA, GL_UNSIGNED_INT, NULL);
    glBindImageTexture(texture_slot, texture_handle, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);

    checkOpenGLErrors("generateTexture");

    return texture_handle;
}

GLuint generateComputeProgram(const char* path) {
    GLuint program = glCreateProgram();
    GLuint compute_shader = glCreateShader(GL_COMPUTE_SHADER);

    std::string shader_source = read_file(path);
    // create and compiler vertex shader
    const char *source = shader_source.c_str();
    int length = shader_source.size();
    glShaderSource(compute_shader, 1, &source, &length);
    glCompileShader(compute_shader);
    if(!check_shader_compile_status(compute_shader)) {
        return 0;
    }

    // attach shaders
    glAttachShader(program, compute_shader);

    // link the program and check for errors
    glLinkProgram(program);
    check_program_link_status(program);

    glDeleteShader(compute_shader);
    checkOpenGLErrors("generateComputeProgram");
    return program;
}

struct Agent {
    float x;
    float y;
    float angle;
    float type;
};

static float fps(double *nbFrames, double *lastTime) {
    // Measure speed
    double currentTime = glfwGetTime();
    double delta = currentTime - *lastTime;
    (*nbFrames)++;
    if ( delta >= 1.0 ){ // If last cout was more than 1 sec ago
        *nbFrames = 0;
        *lastTime += 1.0;
    }
    return (*nbFrames)/delta;
}

int sensitivity_input = 0;
int sensitivity = 20;

static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    if (key == GLFW_KEY_J && action == GLFW_PRESS)
        sensitivity_input--;
    if (key == GLFW_KEY_K && action == GLFW_PRESS)
        sensitivity_input++;

}

int main() {

    PulseAudioContext PACtx = initializeSimplePulseAudio();
    float buffer_left_sound[BUFFER_SIZE];
    float buffer_right_sound[BUFFER_SIZE];
    GLFWwindow *window = initializeOpenGl();

    int texture_slot0 = 0;
    int texture_slot1 = 1;
    GLuint texture0 = generateTexture(texture_slot0);
    GLuint texture1 = generateTexture(texture_slot1);
    // program and shader handles
    GLuint shader_program = generateRenderProgram();
    glUniform1i(glGetUniformLocation(shader_program, "tex"), texture_slot1);

    // create program
    GLuint acceleration_program = generateComputeProgram("../compute_shader.glsl");
    GLuint evaporate_program = generateComputeProgram("../evaporate_shader.glsl");
    GLuint copy_program = generateComputeProgram("../copy_shader.glsl");

    // CREATE INIT DATA

    // randomly place agents
    Agent *AgentsData = (Agent *) malloc(AGENTS_COUNT*4*sizeof(float));
    for(int i = 0;i<AGENTS_COUNT;++i) {
        // initial position
        //AgentsData[i].x = (rand()%WIDTH);
        //AgentsData[i].y = (rand()%HEIGHT);
        AgentsData[i].x = WIDTH/2;
        AgentsData[i].y = HEIGHT/2;
        AgentsData[i].angle = 0 * 3.14f * 2.0f;
        if ((float)i > (float)AGENTS_COUNT/2.0f) {
            AgentsData[i].type = 2;
        }
        //printf("angle data: %f \n", angleData[i]);
        //printf("positions data: %f x %f\n", positionData[index], positionData[index+1]);
    }

    // generate positions_vbos and vaos and general vbo, ibo
    GLuint agents_vbo;

    glGenBuffers(1, &agents_vbo);

    //ORIGINAL STUFF FOR AGENTS
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, agents_vbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float)*4*AGENTS_COUNT, AgentsData, GL_STATIC_DRAW);

    //This is likely for compute shaders
    const GLuint ssbos[] = {agents_vbo};
    glBindBuffersBase(GL_SHADER_STORAGE_BUFFER, 0, 1, ssbos);

    // physical parameters
    float dt = 1.0f/60.0f;

    // setup uniforms
    glUseProgram(acceleration_program);
    glUniform1i(glGetUniformLocation(acceleration_program, "srcTex"), texture_slot1);
    glUniform1i(glGetUniformLocation(acceleration_program, "destTex"), texture_slot0);
    glUniform1i(glGetUniformLocation(acceleration_program, "width"), WIDTH);
    glUniform1i(glGetUniformLocation(acceleration_program, "height"), HEIGHT);
    glUniform1f(glGetUniformLocation(acceleration_program, "moveSpeed"), MOVE_SPEED);
    glUniform1f(glGetUniformLocation(acceleration_program, "turnSpeed"), TURN_SPEED);
    glUniform1f(glGetUniformLocation(acceleration_program, "agentsCount"), AGENTS_COUNT );

    glUniform1f(glGetUniformLocation(acceleration_program, "sensorAngleSpacing"), SENSOR_ANGLE_SPACING);
    glUniform1f(glGetUniformLocation(acceleration_program, "sensorOffsetDst"), SENSOR_OFFSET_DST);
    glUniform1f(glGetUniformLocation(acceleration_program, "sensorSize"), SENSOR_SIZE);

    glUseProgram(evaporate_program);
    glUniform1i(glGetUniformLocation(evaporate_program, "srcTex"), texture_slot0);
    glUniform1i(glGetUniformLocation(evaporate_program, "destTex"), texture_slot1);
    glUniform1f(glGetUniformLocation(evaporate_program, "evaporate_speed"), EVAPORATE_SPEED);
    glUniform1f(glGetUniformLocation(evaporate_program, "diffuse_speed"), DIFFUSE_SPEED);
    glUniform1i(glGetUniformLocation(evaporate_program, "width"), WIDTH);
    glUniform1i(glGetUniformLocation(evaporate_program, "height"), HEIGHT);

    glUseProgram(copy_program);
    glUniform1i(glGetUniformLocation(copy_program, "srcTex"), texture_slot1);
    glUniform1i(glGetUniformLocation(copy_program, "destTex"), texture_slot0);

    // we are blending so no depth testing
    glDisable(GL_DEPTH_TEST);

    // enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);

    glClearColor(0.0, 0.0, 0.0, 1.0);

    //GLuint query;
    //glGenQueries(1, &query);

    double nbFrames = 0;
    double lastTime = 0;

    const char *pipe_file = "./cava_fifo";
    int fd = open(pipe_file, O_RDONLY);

    rewriteConfig(20);
    reloadConfig();

    glfwSetKeyCallback(window, key_callback);

    unsigned char cava_input[30];
    while(!glfwWindowShouldClose(window)) {
        //glBeginQuery(GL_TIME_ELAPSED, query);
        int vals_read = 30;
        while (vals_read == 30){
            vals_read = read(fd, cava_input, 30);
        }

        if (sensitivity_input > 0) {
            sensitivity += 5;
            sensitivity_input--;
            rewriteConfig(sensitivity);
            printf("setting sensitivity: %i\n", sensitivity);
            reloadConfig();
        }
        if (sensitivity_input < 0) {
            sensitivity -= 5;
            sensitivity_input++;
            rewriteConfig(sensitivity);
            printf("setting sensitivity: %i\n", sensitivity);
            reloadConfig();
        }

        unsigned char max = 0;
        for (int i=0;i<30;i++) {
            if (max < cava_input[i]) {
                max = cava_input[i];
            }
           // printf("%u ", cava_input[i]);
        }
        //for (unsigned int i=0;i<max;i++) {
        //    printf("=");
        //}
        float float_max = (float)max / (float)5120* 8;
        //printf("\n");
        //printf("max1: %f\n", float_max);
        //printf("passing to shader: %f\n\n", 0.3f + float_max*MOVE_SPEED);
        max = 0;
        for (int i=15;i<30;i++) {
            if (max < cava_input[i]) {
                max = cava_input[i];
            }
           // printf("%u ", cava_input[i]);
        }
        //for (unsigned int i=0;i<max;i++) {
        //    printf("=");
        //}
        float float_max_type2 = (float) max /(float) 255;

        glfwPollEvents();


        glUseProgram(acceleration_program);
        glUniform1f(glGetUniformLocation(acceleration_program, "moveSpeed"), 0.3f + float_max*MOVE_SPEED);
        glUniform1f(glGetUniformLocation(acceleration_program, "turnSpeed"), 0.6f - float_max*TURN_SPEED);
        glUniform1f(glGetUniformLocation(acceleration_program, "moveSpeedType2"), 0.3 + float_max_type2*MOVE_SPEED);
        glUniform1f(glGetUniformLocation(acceleration_program, "turnSpeedType2"), TURN_SPEED - float_max_type2*TURN_SPEED);
        glUniform1f(glGetUniformLocation(acceleration_program, "dt"), dt);
        glDispatchCompute(WIDTH/8, HEIGHT/8, 1);

        glUseProgram(evaporate_program);
        glUniform1f(glGetUniformLocation(evaporate_program, "dt"), dt);
        glDispatchCompute(WIDTH/32, HEIGHT/32, 1);

        glUseProgram(copy_program);
        glDispatchCompute(WIDTH/16, HEIGHT/16, 1);

        // clear first
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // use the shader program
        glUseProgram(shader_program);

        // draw
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        checkOpenGLErrors("Main Loop");

        // finally swap buffers
        glfwSwapBuffers(window);
        //glEndQuery(GL_TIME_ELAPSED);

        //{
        //    GLuint64 result;
        //    glGetQueryObjectui64v(query, GL_QUERY_RESULT, &result);
        //    std::cout << result*1.e-6 << " ms/frame" << std::endl;
        //}
        float fps_v = fps(&nbFrames, &lastTime);
        //printf("fps: %f\n", fps_v);
        float dt = 1.0f/fps_v;
    }

    // delete the created objects
    glDeleteTextures(1, &texture0);
    glDeleteTextures(1, &texture1);

    //glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &agents_vbo);

    glDeleteProgram(acceleration_program);
    glDeleteProgram(evaporate_program);
    glDeleteProgram(copy_program);

    glfwDestroyWindow(window);
    glfwTerminate();

    destroySimplePulseAudio(PACtx);
    free(AgentsData);

    return 0;
}
