
#include "util.hpp"

std::string read_file(std::string path) {

  std::ifstream ifs(path);
  std::string content( (std::istreambuf_iterator<char>(ifs) ),
                       (std::istreambuf_iterator<char>()    ) );

  return content;
}

void checkOpenGLErrors(const char *location) {
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        fprintf(stderr, "OpenGL error from: %s: %s (%d)\n", location, gluErrorString(err), err);
        exit(-1);
    }
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
