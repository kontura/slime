#define GLEW_STATIC
#include <GL/glew.h>

#include <GLFW/glfw3.h>

#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <cmath>
#include <fstream>

#define PARTICLES_COUNT 16*1024

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

int main() {
    int width = 1920;
    int height = 1080;

    if(glfwInit() == GL_FALSE) {
        std::cerr << "failed to init GLFW" << std::endl;
        return 1;
    }

    // select opengl version
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    // create a window
    GLFWwindow *window;
    if((window = glfwCreateWindow(width, height, "handmade_slime", 0, 0)) == 0) {
        std::cerr << "failed to open window" << std::endl;
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);

    if(glewInit()) {
        std::cerr << "failed to init GL3W" << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    // shader source code

    // shader source code
    std::string vertex_source =
        "#version 330\n"
        "layout(location = 0) in vec4 vposition;\n"
        "layout(location = 1) in vec4 vcolor;\n"
        "out vec4 fcolor;\n"
        "void main() {\n"
        "   fcolor = vcolor;\n"
        "   gl_Position = vposition;\n"
        "}\n";

    std::string fragment_source =
        "#version 330\n"
        "uniform sampler2D tex;\n" // texture uniform
        "in vec2 ftexcoord;\n"
        "in vec4 fcolor;\n"
        "layout(location = 0) out vec4 FragColor;\n"
        "void main() {\n"
        //"   FragColor = vec4(0,1,1,1);\n"
        "   FragColor = texture(tex, ftexcoord);\n"
        "}\n";

    // program and shader handles
    GLuint shader_program, vertex_shader, fragment_shader;

    // we need these to properly pass the strings
    const char *source;
    int length;

    // create and compiler vertex shader
    vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    source = vertex_source.c_str();
    length = vertex_source.size();
    glShaderSource(vertex_shader, 1, &source, &length);
    glCompileShader(vertex_shader);
    if(!check_shader_compile_status(vertex_shader)) {
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    // create and compiler fragment shader
    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    source = fragment_source.c_str();
    length = fragment_source.size();
    glShaderSource(fragment_shader, 1, &source, &length);
    glCompileShader(fragment_shader);
    if(!check_shader_compile_status(fragment_shader)) {
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    // create program
    shader_program = glCreateProgram();

    // attach shaders
    glAttachShader(shader_program, vertex_shader);
    glAttachShader(shader_program, fragment_shader);

    // link the program and check for errors
    glLinkProgram(shader_program);
    check_program_link_status(shader_program);

    // get texture uniform location
    GLint texture_location = glGetUniformLocation(shader_program, "tex");

    std::string acceleration_source = read_file("../compute_shader");

    // program and shader handles
    GLuint acceleration_program, acceleration_shader;

    // create and compiler vertex shader
    acceleration_shader = glCreateShader(GL_COMPUTE_SHADER);
    source = acceleration_source.c_str();
    length = acceleration_source.size();
    glShaderSource(acceleration_shader, 1, &source, &length);
    glCompileShader(acceleration_shader);
    if(!check_shader_compile_status(acceleration_shader)) {
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    // create program
    acceleration_program = glCreateProgram();

    // attach shaders
    glAttachShader(acceleration_program, acceleration_shader);

    // link the program and check for errors
    glLinkProgram(acceleration_program);
    check_program_link_status(acceleration_program);

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
    source = integrate_source.c_str();
    length = integrate_source.size();
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


    //TODO(amatej): why is this so slow? rather why it lags the pc when a tad larger?
    //          ->> its because we iterate from every elem to every elem
    //create textures where agents will draw and that will ultimately be displayed
    // texture handle
    GLuint texture;
    // generate texture
    glGenTextures(1, &texture);
    // bind the texture
    glBindTexture(GL_TEXTURE_2D, texture);
    // create some image data
    std::vector<GLubyte> image(4*width*height);
    for(int j = 0;j<height;++j) {
        for(int i = 0;i<width;++i) {
            size_t index = j*width + i;
            image[4*index + 0] = 0xFF*(j/10%2)*(i/10%2); // R
            image[4*index + 1] = 0xFF*(j/13%2)*(i/13%2); // G
            image[4*index + 2] = 0xFF*(j/17%2)*(i/17%2); // B
            image[4*index + 3] = 0xFF;                   // A
        }
    }
    // set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // set texture content
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, &image[0]);

    // randomly place particles in a cube
    float positionData[4*PARTICLES_COUNT];
    float velocityData[4*PARTICLES_COUNT];
    for(int i = 0;i<PARTICLES_COUNT;++i) {
        // initial position
        int index = i * 4;
        positionData[index] = ((rand()%2000)-1000)/(float)2000;
        positionData[index+1] = ((rand()%2000)-1000)/(float)2000;
        positionData[index+2] = 0;
        positionData[index+3] = 1;
    }

    // generate positions_vbos and vaos
    GLuint vao, positions_vbo, velocities_vbo;

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &positions_vbo);
    glGenBuffers(1, &velocities_vbo);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, velocities_vbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float)*4*PARTICLES_COUNT, velocityData, GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, positions_vbo);

    // fill with initial data
    //glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4)*PARTICLES_COUNT, &positionData[0], GL_STATIC_DRAW);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float)*4*PARTICLES_COUNT, positionData, GL_STATIC_DRAW);

    // set up generic attrib pointers
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4*sizeof(GLfloat), (char*)0 + 0*sizeof(GLfloat));

    const GLuint ssbos[] = {positions_vbo, velocities_vbo};
    glBindBuffersBase(GL_SHADER_STORAGE_BUFFER, 0, 2, ssbos);

    // physical parameters
    float dt = 1.0f/60.0f;

    // setup uniforms
    glUseProgram(acceleration_program);
    glUniform1f(0, dt);

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

        // bind texture to texture unit 0
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        
        // set texture uniform
        glUniform1i(texture_location, 0);

        // bind the current vao
        glBindVertexArray(vao);

        // draw
        glDrawArrays(GL_POINTS, 0, PARTICLES_COUNT);

        // check for errors
        GLenum error = glGetError();
        if(error != GL_NO_ERROR) {
            std::cerr << error << std::endl;
            break;
        }

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

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &positions_vbo);
    glDeleteBuffers(1, &velocities_vbo);

    glDetachShader(shader_program, vertex_shader);
    glDetachShader(shader_program, fragment_shader);
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    glDeleteProgram(shader_program);

    glDetachShader(acceleration_program, acceleration_shader);
    glDeleteShader(acceleration_shader);
    glDeleteProgram(acceleration_program);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
