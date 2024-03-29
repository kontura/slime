#include <string.h>
#include <cstdio>

#include "input_pulse.hpp"

pa_mainloop *m_pulseaudio_mainloop;

void cb(__attribute__((unused)) pa_context *pulseaudio_context, const pa_server_info *i, void *userdata) {
    // getting default sink name
    PulseAudioContext *audio = (PulseAudioContext *)userdata;
    free(audio->source);
    audio->source = (char*) malloc(sizeof(char) * 1024);

    strcpy(audio->source, i->default_sink_name);

    // appending .monitor suufix
    audio->source = strcat(audio->source, ".monitor");

    // quiting mainloop
    pa_context_disconnect(pulseaudio_context);
    pa_context_unref(pulseaudio_context);
    pa_mainloop_quit(m_pulseaudio_mainloop, 0);
    pa_mainloop_free(m_pulseaudio_mainloop);
}

void pulseaudio_context_state_callback(pa_context *pulseaudio_context, void *userdata) {
    // make sure loop is ready
    switch (pa_context_get_state(pulseaudio_context)) {
    case PA_CONTEXT_UNCONNECTED:
        //printf("UNCONNECTED\n");
        break;
    case PA_CONTEXT_CONNECTING:
        //printf("CONNECTING\n");
        break;
    case PA_CONTEXT_AUTHORIZING:
        //printf("AUTHORIZING\n");
        break;
    case PA_CONTEXT_SETTING_NAME:
        //printf("SETTING_NAME\n");
        break;
    case PA_CONTEXT_READY: // extract default sink name
        //printf("READY\n");
        pa_operation_unref(pa_context_get_server_info(pulseaudio_context, cb, userdata));
        break;
    case PA_CONTEXT_FAILED:
        fprintf(stderr, "failed to connect to pulseaudio server\n");
        exit(EXIT_FAILURE);
        break;
    case PA_CONTEXT_TERMINATED:
        //printf("TERMINATED\n");
        pa_mainloop_quit(m_pulseaudio_mainloop, 0);
        break;
    }
}

void initialize_pulse(void *userdata) {
    pa_mainloop_api *mainloop_api;
    pa_context *pulseaudio_context;
    int ret;

    // Create a mainloop API and connection to the default server
    m_pulseaudio_mainloop = pa_mainloop_new();

    mainloop_api = pa_mainloop_get_api(m_pulseaudio_mainloop);
    pulseaudio_context = pa_context_new(mainloop_api, "slime device list");

    // This function connects to the pulse server
    pa_context_connect(pulseaudio_context, NULL, PA_CONTEXT_NOFLAGS, NULL);

    //        printf("connecting to server\n");

    // This function defines a callback so the server will tell us its state.
    pa_context_set_state_callback(pulseaudio_context, pulseaudio_context_state_callback, userdata);

    // starting a mainloop to get default sink

    // starting with one nonblokng iteration in case pulseaudio is not able to run
    if (!(ret = pa_mainloop_iterate(m_pulseaudio_mainloop, 0, &ret))) {
        fprintf(stderr,
                "Could not open pulseaudio mainloop to "
                "find default device name: %d\n"
                "check if pulseaudio is running\n",
                ret);

        exit(EXIT_FAILURE);
    }

    pa_mainloop_run(m_pulseaudio_mainloop, &ret);

    struct PulseAudioContext *audio = (struct PulseAudioContext *)userdata;

    /* The sample type to use */
    static const pa_sample_spec ss = {.format = PA_SAMPLE_S16LE, .rate = 44100, .channels = 2};
    const int frag_size = FFTW_SAMPLES * 2 * 16 / 8 * 2; // we double this because of cpu performance issues with pulseaudio

    pa_buffer_attr pb = {.maxlength = (uint32_t)-1, // BUFSIZE * 2,
                         .tlength = 0, //playback only, added just to supress warning of missing initializer
                         .prebuf = 0, //playback only, added just to supress warning of missing initializer
                         .minreq = 0, //playback only, added just to supress warning of missing initializer
                         .fragsize = frag_size};

    int error;
    if (!(audio->s = pa_simple_new(NULL, "slime", PA_STREAM_RECORD, audio->source, "audio for slime", &ss, NULL, &pb, &error))) {
        fprintf(stderr, __FILE__ ": Could not open pulseaudio source: %s, %s. \
		To find a list of your pulseaudio sources run 'pacmd list-sources'\n",
                audio->source, pa_strerror(error));

        exit(EXIT_FAILURE);
    }
}

void *input_pulse(void *data) {
    int error;

    struct PulseAudioContext *audio = (struct PulseAudioContext *)data;
    if (pa_simple_flush(audio->s, &error)) {
        fprintf(stderr, __FILE__ ": pa_simple_flush() failed: %s\n", pa_strerror(error));
        exit(EXIT_FAILURE);
    }
    if (pa_simple_read(audio->s, audio->buf, sizeof(audio->buf), &error) < 0) {
        fprintf(stderr, __FILE__ ": pa_simple_read() failed: %s\n", pa_strerror(error));
        exit(EXIT_FAILURE);
    }

    return 0;
}
