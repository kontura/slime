#define GLEW_STATIC
#include <GL/glew.h>

#include <GLFW/glfw3.h>

#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <cmath>
#include <fstream>

#define PARTICLES_COUNT 1024
#define WIDTH 1920
#define HEIGHT 1080


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
    glUniform1i(glGetUniformLocation(program, "tex"), 0);

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


    checkOpenGLErrors("generateRenderProgram");

    return program;
}

GLuint generateTexture() {
    GLuint texture_handle;
    glGenTextures(1, &texture_handle);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_handle);

    // set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // set texture content
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, WIDTH, HEIGHT, 0, GL_RED, GL_FLOAT, NULL);
    glBindImageTexture(0, texture_handle, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F);

    checkOpenGLErrors("generateTexture");

    return texture_handle;
}

GLuint generateComputeProgram() {
    GLuint program = glCreateProgram();
    GLuint compute_shader = glCreateShader(GL_COMPUTE_SHADER);

    std::string acceleration_source = read_file("../compute_shader");
    // create and compiler vertex shader
    const char *source = acceleration_source.c_str();
    int length = acceleration_source.size();
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

    checkOpenGLErrors("generateComputeProgram");
    return program;
}

int main() {
    GLFWwindow *window = initializeOpenGl();

    GLuint texture = generateTexture();
    // program and shader handles
    GLuint shader_program = generateRenderProgram();

    // get texture uniform location
    GLint texture_location = glGetUniformLocation(shader_program, "tex");


    // create program
    GLuint acceleration_program = generateComputeProgram();


    // the integrate shader does the second part of the euler integration
    std::string integrate_source =
        "#version 430\n"
        "layout(local_size_x=256) in;\n"

        "layout(location = 0) uniform float dt;\n"
        "layout(std430, binding=0) buffer pblock { vec4 positions[]; };\n"
        "layout(std430, binding=1) buffer vblock { vec4 velocities[]; };\n"

        "void main() {\n"
        "   int index = int(gl_GlobalInvocationID);\n"
        "   vec4 position = positions[index];\n"
        "   vec4 velocity = velocities[index];\n"
        "   position.xyz += dt*velocity.xyz;\n"
        "   if (position.x > 0.5) {"
        "       position.x = 0.5;"
        "       velocities[index] = -velocities[index];"
        "   }"
        "   if (position.x < -0.5) {"
        "       position.x = -0.5;"
        "   }"
        "   if (position.y > 0.5) {"
        "       position.y = 0.5;"
        "       velocities[index] = -velocities[index];"
        "   }"
        "   if (position.y < -0.5) {"
        "       position.y = -0.5;"
        "   }"
        "   position.z = 0;"
        "   positions[index] = position;\n"
        "}\n";

    // program and shader handles
    GLuint integrate_program, integrate_shader;

    // create and compiler vertex shader
    integrate_shader = glCreateShader(GL_COMPUTE_SHADER);
    const char *source = integrate_source.c_str();
    int length = integrate_source.size();
    glShaderSource(integrate_shader, 1, &source, &length);
    glCompileShader(integrate_shader);
    if(!check_shader_compile_status(integrate_shader)) {
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    // create program
    integrate_program = glCreateProgram();

    // attach shaders
    glAttachShader(integrate_program, integrate_shader);

    // link the program and check for errors
    glLinkProgram(integrate_program);
    check_program_link_status(integrate_program);


    // CREATE INIT DATA

    // randomly place agents
    float positionData[4*PARTICLES_COUNT];
    float velocityData[4*PARTICLES_COUNT];
    for(int i = 0;i<PARTICLES_COUNT;++i) {
        // initial position
        int index = i * 4;
        positionData[index] = ((rand()%2000)-1000)/(float)2000;
        positionData[index+1] = ((rand()%2000)-1000)/(float)2000;
        positionData[index+2] = 0;
        positionData[index+3] = 0;
    }

    // generate positions_vbos and vaos and general vbo, ibo
    GLuint positions_vbo, velocities_vbo;

    glGenBuffers(1, &positions_vbo);
    glGenBuffers(1, &velocities_vbo);

    //ORIGINAL STUFF FOR AGENTS
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, velocities_vbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float)*4*PARTICLES_COUNT, velocityData, GL_STATIC_DRAW);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, positions_vbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float)*4*PARTICLES_COUNT, positionData, GL_STATIC_DRAW);

    // set up generic attrib pointers
    //glEnableVertexAttribArray(0);
    //glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4*sizeof(GLfloat), (char*)0 + 0*sizeof(GLfloat));

    //This is likely for compute shaders
    const GLuint ssbos[] = {positions_vbo, velocities_vbo};
    glBindBuffersBase(GL_SHADER_STORAGE_BUFFER, 0, 2, ssbos);

    // physical parameters
    float dt = 1.0f/60.0f;

    //TODO(amatej): if I want to have the generateComputeProgram general (I need to pass in
    //path to the compute shader it self) and also set uniforms outside?
    // setup uniforms
    glUseProgram(acceleration_program);
    glUniform1f(0, dt);
    glUniform1i(glGetUniformLocation(acceleration_program, "destTest"), 0);

    glUseProgram(integrate_program);
    glUniform1f(0, dt);

    // we are blending so no depth testing
    glDisable(GL_DEPTH_TEST);

    // enable blending
    glEnable(GL_BLEND);
    //  and set the blend function to result = 1*source + 1*destination
    glBlendFunc(GL_ONE, GL_ONE);

    GLuint query;
    glGenQueries(1, &query);

    while(!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        //std::cout << positionData[1000].y << std::endl;

        glBeginQuery(GL_TIME_ELAPSED, query);

        glUseProgram(acceleration_program);
        glDispatchCompute(PARTICLES_COUNT/256, 1, 1);

        glEndQuery(GL_TIME_ELAPSED);

        glUseProgram(integrate_program);

        glDispatchCompute(PARTICLES_COUNT/256, 1, 1);

        // clear first
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // use the shader program
        glUseProgram(shader_program);

        // set texture uniform
        glUniform1i(texture_location, 0);

        // draw
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        checkOpenGLErrors("Main Loop");

        // finally swap buffers
        glfwSwapBuffers(window);

        {
            GLuint64 result;
            glGetQueryObjectui64v(query, GL_QUERY_RESULT, &result);
            //std::cout << result*1.e-6 << " ms/frame" << std::endl;
        }
    }

    // delete the created objects
    glDeleteTextures(1, &texture);

    //glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &positions_vbo);
    glDeleteBuffers(1, &velocities_vbo);

//    glDetachShader(shader_program, vertex_shader);
//    glDetachShader(shader_program, fragment_shader);
//    glDeleteShader(vertex_shader);
//    glDeleteShader(fragment_shader);
//    glDeleteProgram(shader_program);

    //glDetachShader(acceleration_program, acceleration_shader);
    //glDeleteShader(acceleration_shader);
    glDeleteProgram(acceleration_program);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
