#define GLEW_STATIC
#include <GL/glew.h>

#include <GLFW/glfw3.h>

#include <unistd.h>
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <string.h>
#include <fcntl.h>


//for fifo
#include <sys/types.h>
#include <sys/stat.h>

#include "input_pulse.hpp"
#include "ffmpeg_decoder.hpp"
#include "ffmpeg_encoder.hpp"
#include "slime.hpp"
#include "spectrum.hpp"
#include "fluid.hpp"
#include "consts.hpp"
#include "util.hpp"

#include <fftw3.h>

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

static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    (void)scancode;
    (void)mods;
    if ((key == GLFW_KEY_ESCAPE || key == GLFW_KEY_Q) && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
}

void window_close_callback(GLFWwindow* window) {
    (void)window;
}

enum graphic_mode{SLIME_MODE, SPECTRUM_MODE, FLUID_MODE};

int main(int argc, char **argv) {
    graphic_mode mode = SLIME_MODE;
    char *cmdline_file_input_path = NULL;
    int opt;

    while ((opt = getopt(argc, argv, "f:lsd")) != -1) {
        switch (opt) {
            case 'f':
                cmdline_file_input_path = optarg;
                break;
            case 'l':
                mode = SLIME_MODE;
                break;
            case 's':
                mode = SPECTRUM_MODE;
                break;
            case 'd':
                mode = FLUID_MODE;
                break;
            default:
                fprintf(stderr, "Usage: %s [-f FILE] [-l] [-s]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    void (*init_proc)(void *, int);
    void (*run_proc)(void * mode_data,
                     fftw_complex * out_complex_l,
                     float dt,
                     int tx0,
                     int tx1,
                     int tx2,
                     int tx3,
                     int tx4,
                     int tx5);
    void (*finalize_proc)(void *);
    void * mode_data;

    switch (mode) {
        case SPECTRUM_MODE:
            init_proc = &initialize_spectrum;
            run_proc = &run_spectrum;
            finalize_proc = &finalize_spectrum;
            mode_data = (void*)malloc(sizeof(Spectrum));
            break;
        case FLUID_MODE:
            init_proc = &initialize_fluid;
            run_proc = &run_fluid;
            finalize_proc = &finalize_fluid;
            mode_data = (void*)malloc(sizeof(Fluid));
            break;
        default:
        case SLIME_MODE:
            init_proc = &initialize_slime;
            run_proc = &run_slime;
            finalize_proc = &finalize_slime;
            mode_data = (void*)malloc(sizeof(Slime));
            break;


    }


    int tex_order = 1;

    GLFWwindow *window = initializeOpenGl();

    int texture_slot0 = 0;
    int texture_slot1 = 1;
    int texture_slot2 = 2;
    int texture_slot3 = 3;
    int texture_slot4 = 4;
    int texture_slot5 = 5;
    GLuint texture0 = generateTexture(texture_slot0);
    GLuint texture1 = generateTexture(texture_slot1);
    GLuint texture2 = generateTexture(texture_slot2);
    GLuint texture3 = generateTexture(texture_slot3);
    GLuint texture4 = generateTexture(texture_slot4);
    GLuint texture5 = generateTexture(texture_slot5);

    // program and shader handles
    GLuint shader_program = generateRenderProgram();

    init_proc(mode_data, AGENTS_COUNT);

    // we are blending so no depth testing
    glDisable(GL_DEPTH_TEST);

    // enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);

    glClearColor(0.0, 0.0, 0.0, 1.0);

    //GLuint query;
    //glGenQueries(1, &query);

    glfwSetKeyCallback(window, key_callback);
    glfwSetWindowCloseCallback(window, window_close_callback);

    // load and decode song
    Decoder *ffmpeg_decoder = NULL;
    Encoder *ffmpeg_encoder = NULL;
    double *in_raw_fftw_l = NULL;
    double *in_raw_fftw_r = NULL;
    fftw_plan fftw_plan_l, fftw_plan_r;
    fftw_complex *out_complex_l, *out_complex_r;
    PulseAudioContext pac = {0};
    in_raw_fftw_l = fftw_alloc_real(FFTW_BUFFER_SIZE);
    in_raw_fftw_r = fftw_alloc_real(FFTW_BUFFER_SIZE);
    out_complex_l = fftw_alloc_complex(FFTW_BUFFER_SIZE/2 + 1);
    out_complex_r = fftw_alloc_complex(FFTW_BUFFER_SIZE/2 + 1);
    memset(out_complex_l, 0, (FFTW_BUFFER_SIZE/2 + 1) * sizeof(fftw_complex));
    memset(out_complex_r, 0, (FFTW_BUFFER_SIZE/2 + 1) * sizeof(fftw_complex));
    fftw_plan_l = fftw_plan_dft_r2c_1d(FFTW_BUFFER_SIZE, in_raw_fftw_l, out_complex_l, FFTW_MEASURE);
    fftw_plan_r = fftw_plan_dft_r2c_1d(FFTW_BUFFER_SIZE, in_raw_fftw_r, out_complex_r, FFTW_MEASURE);
    if (cmdline_file_input_path != NULL) {
        ffmpeg_decoder = decoder_new(cmdline_file_input_path);

        const char *my_suffix = "_slime.m4a";
        char *last_dot_in_input = strrchr(cmdline_file_input_path, '.');
        *last_dot_in_input = '\0';
        size_t len = strlen(cmdline_file_input_path) + strlen(my_suffix);
        char *dest_name = (char*)malloc(len);
        sprintf(dest_name, "%s%s", cmdline_file_input_path, my_suffix);

        ffmpeg_encoder = encoder_new(dest_name);
        free(dest_name);
    } else {
        initialize_pulse((void *) &pac);
    }

    unsigned char *pic = (unsigned char*) malloc(WIDTH*HEIGHT*3);

    float dt = OFFLINE_DT;
    double last_time = 0;
    double current_time = 0;
    while(!glfwWindowShouldClose(window)) {
        current_time = glfwGetTime();
        //glBeginQuery(GL_TIME_ELAPSED, query);
        if (cmdline_file_input_path) {
            if (ffmpeg_decoder->samples_buffer_count <= FFTW_BUFFER_BYTES) {
                load_audio_samples(ffmpeg_decoder);
            }
            int16_t *input = (int16_t*) ffmpeg_decoder->samples_buffer;
            for (int i = 0; i<FFTW_SAMPLES*2; i+=2) {
                in_raw_fftw_l[i/2] = input[i];
                //TODO(amatej): maybe do both left and right .. but for now.. good enough
            }
        } else {
            dt = current_time - last_time;
            input_pulse(&pac);
            for (int i = 0; i<FFTW_SAMPLES*2; i+=2) {
                in_raw_fftw_l[i/2] = pac.buf[i];
                //TODO(amatej): maybe do both left and right .. but for now.. good enough
            }
        }
        fftw_execute(fftw_plan_l);

        run_proc(mode_data, out_complex_l, dt, texture_slot0, texture_slot1, texture_slot2, texture_slot3, texture_slot4, texture_slot5);

        glfwPollEvents();

        // clear first
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // use the shader program
        glUseProgram(shader_program);
        if (tex_order) {
            glUniform1i(glGetUniformLocation(shader_program, "tex"), texture_slot0);
        } else {
            glUniform1i(glGetUniformLocation(shader_program, "tex"), texture_slot1);
        }

        // draw
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        checkOpenGLErrors("Main Loop");

        //capture video
        if (cmdline_file_input_path) {
            if (av_frame_make_writable(ffmpeg_encoder->video_st.frame) < 0) {
                fprintf(stderr, "Failed to make encoder frame writable \n");
                exit(1);
            }

            glReadPixels(0, 0, WIDTH, HEIGHT, GL_RGB, GL_UNSIGNED_BYTE, pic);
            SwsContext * ctx = sws_getContext(WIDTH, HEIGHT, AV_PIX_FMT_RGB24,
                                              ffmpeg_encoder->video_st.enc->width,
                                              ffmpeg_encoder->video_st.enc->height,
                                              AV_PIX_FMT_YUV420P, 0, 0, 0, 0);
            uint8_t * inData[1] = { pic }; // RGB24 have one plane
            int inLinesize[1] = { 3*WIDTH }; // RGB stride
            sws_scale(ctx, inData, inLinesize, 0, HEIGHT,
                      ffmpeg_encoder->video_st.frame->data,
                      ffmpeg_encoder->video_st.frame->linesize);
            ffmpeg_encoder->video_st.frame->pts = ffmpeg_encoder->video_st.next_pts++;

            write_frame(ffmpeg_encoder->output_context,
                        ffmpeg_encoder->video_st.enc,
                        ffmpeg_encoder->video_st.st,
                        ffmpeg_encoder->video_st.frame,
                        ffmpeg_encoder->video_st.tmp_pkt);
            write_audio_frame(ffmpeg_decoder, ffmpeg_encoder->output_context, &(ffmpeg_encoder->audio_st));

            // move audio samples
            ffmpeg_decoder->samples_buffer_count -= FFTW_BUFFER_BYTES;
            memmove(ffmpeg_decoder->samples_buffer,
                    ffmpeg_decoder->samples_buffer+FFTW_BUFFER_BYTES,
                    ffmpeg_decoder->samples_buffer_count);

            if (ffmpeg_decoder->samples_buffer_count <= 0) {
                glfwSetWindowShouldClose(window, GL_TRUE);
            }
        }

        // finally swap buffers
        glfwSwapBuffers(window);
        //glEndQuery(GL_TIME_ELAPSED);

        //{
        //    GLuint64 result;
        //    glGetQueryObjectui64v(query, GL_QUERY_RESULT, &result);
        //    std::cout << result*1.e-6 << " ms/frame" << std::endl;
        //}

        last_time = current_time;
    }

    free(pic);

    if (cmdline_file_input_path != NULL) {
        /* flush the decoder */
        ffmpeg_decoder->packet->data = NULL;
        ffmpeg_decoder->packet->size = 0;
        //TODO(amatej): this just adds bytes to samples_buffer nothing is processing them...
        decode(ffmpeg_decoder);

        decoder_free(ffmpeg_decoder);
        encoder_free(ffmpeg_encoder);

    } else {
        pa_simple_free(pac.s);
        free(pac.source);
    }
    fftw_free(in_raw_fftw_l);
    fftw_free(in_raw_fftw_r);

    fftw_destroy_plan(fftw_plan_l);
    fftw_destroy_plan(fftw_plan_r);

    fftw_free(out_complex_l);
    fftw_free(out_complex_r);

    // delete the created objects
    glDeleteTextures(1, &texture0);
    glDeleteTextures(1, &texture1);
    glDeleteTextures(1, &texture2);
    glDeleteTextures(1, &texture3);
    glDeleteTextures(1, &texture4);
    glDeleteTextures(1, &texture5);

    //glDeleteVertexArrays(1, &vao);

    glfwDestroyWindow(window);
    glfwTerminate();

    finalize_proc(mode_data);
    free(mode_data);

    return 0;
}
