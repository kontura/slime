#include "capture.hpp"
#include <sys/stat.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

typetga tga;

char captureName[256];
unsigned long captureNumber;
unsigned char tgahead[12] = {0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

int capture(GLFWwindow *window) {
    int screenWidth, screenHeight;
    glfwGetFramebufferSize(window, &screenWidth, &screenHeight);

    memcpy(tga.head, tgahead, 12);

    tga.dx = screenWidth;
    tga.dy = screenHeight;
    tga.pix_depth = 24;
    tga.img_desc = 0;

    /* Read pixels from [0, 0] to screenWidth, screenHeight, mode (RGB), type (3 bytes RGB), store into tga.pic */
    glReadPixels(0, 0, screenWidth, screenHeight, GL_BGR, GL_UNSIGNED_BYTE, tga.pic[0]);

    mkdir("./capture", 0777);

    /* Store "Capture_%04lu.tga" + captureNumber into captureName, increase frame count */
    sprintf(captureName, "./capture/Capture_%04lu.tga" /* 'lu' for unsigned long */, captureNumber); captureNumber++;

    /* Write file */
    FILE* cc = fopen(captureName, "wb");
    fwrite(&tga, 1, (18 + 3 * screenWidth * screenHeight), cc);
    fclose(cc);

    return 1;
}
