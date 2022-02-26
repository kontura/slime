#pragma once

#include <GLFW/glfw3.h>

typedef struct {
    unsigned char head[12];
    unsigned short dx, dy;
    unsigned char pix_depth;
    unsigned char img_desc;
    unsigned char pic[768 * 1024 * 10][3];
} typetga;



int capture(GLFWwindow *window);
