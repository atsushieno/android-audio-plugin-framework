
#include <aap/android-audio-plugin.h>
#include <cstring>

extern "C" {

typedef struct {
    /* any kind of extension information, which will be passed as void* */
    float modL{0.5}, modR{0.5};
    int32_t delayL{0}, delayR{256};
} SamplePluginSpecific;

void sample_plugin_delete(
        AndroidAudioPluginFactory *pluginFactory,
        AndroidAudioPlugin *instance) {
    delete (SamplePluginSpecific*) instance->plugin_specific;
    delete instance;
}

void sample_plugin_prepare(AndroidAudioPlugin *plugin, AndroidAudioPluginBuffer *buffer) {
    /* do something */
}

void sample_plugin_activate(AndroidAudioPlugin *plugin) {}

void sample_plugin_process(AndroidAudioPlugin *plugin,
                           AndroidAudioPluginBuffer *buffer,
                           long timeoutInNanoseconds) {

    auto p = (SamplePluginSpecific*) plugin->plugin_specific;

    // apply super-simple delay processing
    int size = buffer->num_frames * sizeof(float);

    auto audioInL = (float *) buffer->buffers[0];
    auto audioInR = (float *) buffer->buffers[1];
    auto audioOutL = (float *) buffer->buffers[2];
    auto audioOutR = (float *) buffer->buffers[3];
    auto midiIn = (float *) buffer->buffers[4];
    auto midiOut = (float *) buffer->buffers[5];
    for (int i = 0; i < size / sizeof(float); i++) {
        if (i >= p->delayL)
            audioOutL[i] = (float) (audioInL[i - p->delayL] * p->modL);
        if (i >= p->delayR)
            audioOutR[i] = (float) (audioInR[i - p->delayR] * p->modR);
    }

    /* do anything. In this example, inputs (0,1) are directly copied to outputs (2,3) */
    /*
    memcpy(buffer->buffers[2], buffer->buffers[0], size);
    memcpy(buffer->buffers[3], buffer->buffers[1], size);
    */

    // skip dummy parameter (buffers[4])
}

void sample_plugin_deactivate(AndroidAudioPlugin *plugin) {}

void sample_plugin_get_state(AndroidAudioPlugin *plugin, AndroidAudioPluginState *state) {
    // FIXME: implement
}

void sample_plugin_set_state(AndroidAudioPlugin *plugin, AndroidAudioPluginState *input) {
    // FIXME: implement
}

AndroidAudioPlugin *sample_plugin_new(
        AndroidAudioPluginFactory *pluginFactory,
        const char *pluginUniqueId,
        int sampleRate,
        AndroidAudioPluginExtension **extensions) {
    return new AndroidAudioPlugin{
            new SamplePluginSpecific{},
            sample_plugin_prepare,
            sample_plugin_activate,
            sample_plugin_process,
            sample_plugin_deactivate,
            sample_plugin_get_state,
            sample_plugin_set_state
    };
}

AndroidAudioPluginFactory factory{sample_plugin_new, sample_plugin_delete};

AndroidAudioPluginFactory *GetAndroidAudioPluginFactory() {
    return &factory;
}

}
