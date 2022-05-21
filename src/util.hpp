#pragma once

#define GLEW_STATIC
#include <GL/glew.h>

#include <GLFW/glfw3.h>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>

std::string read_file(std::string path);
void checkOpenGLErrors(const char *location);
bool check_shader_compile_status(GLuint obj);
bool check_program_link_status(GLuint obj);
GLuint generateComputeProgram(const char* path);
