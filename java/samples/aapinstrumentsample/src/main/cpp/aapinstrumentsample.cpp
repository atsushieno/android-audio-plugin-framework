
#include <aap/android-audio-plugin.h>
#include <cstring>
#include <math.h>

extern "C" {

#include "ayumi.h"
#include "cmidi2.h"
#include "aap/aap-midi2.h"

#define AYUMI_AAP_MIDI2_IN_PORT 0
#define AYUMI_AAP_MIDI2_OUT_PORT 1
#define AYUMI_AAP_AUDIO_OUT_LEFT 2
#define AYUMI_AAP_AUDIO_OUT_RIGHT 3
#define AYUMI_AAP_MIDI_CC_ENVELOPE_H 0x10
#define AYUMI_AAP_MIDI_CC_ENVELOPE_M 0x11
#define AYUMI_AAP_MIDI_CC_ENVELOPE_L 0x12
#define AYUMI_AAP_MIDI_CC_ENVELOPE_SHAPE 0x13
#define AYUMI_AAP_MIDI_CC_DC 0x50

typedef struct {
    struct ayumi* impl;
    int mixer[3];
    int32_t envelope;
    int32_t pitchbend;
    double sample_rate;
    bool active;
    bool note_on_state[3];
    int32_t midi_protocol;
} AyumiHandle;


void sample_plugin_delete(
        AndroidAudioPluginFactory *pluginFactory,
        AndroidAudioPlugin *instance) {
    auto context = (AyumiHandle*) instance->plugin_specific;
    delete context->impl;
    delete context;
    delete instance;
}

void sample_plugin_prepare(AndroidAudioPlugin *plugin, AndroidAudioPluginBuffer *buffer) {
    /* do something */
}

void sample_plugin_activate(AndroidAudioPlugin *plugin) {
    auto context = (AyumiHandle*) plugin->plugin_specific;
    context->active = true;
}


double key_to_freq(double key) {
    // We use equal temperament
    // https://pages.mtu.edu/~suits/NoteFreqCalcs.html
    double ret = 220.0 * pow(1.059463, key - 45.0);
    return ret;
}

void ayumi_aap_process_midi_event(AyumiHandle *a, uint8_t *ev) {
    int noise, tone_switch, noise_switch, env_switch;
    uint8_t * msg = ev + 1;
    int channel = msg[0] & 0xF;
    if (channel > 2)
        return;
    int mixer;
    switch (*msg & 0xF0) {
        case CMIDI2_STATUS_NOTE_OFF: note_off:
            if (!a->note_on_state[channel])
                break; // not at note on state
            ayumi_set_mixer(a->impl, channel, 1, 1, 0);
            a->note_on_state[channel] = false;
            break;
        case CMIDI2_STATUS_NOTE_ON:
            if (msg[2] == 0)
                goto note_off; // it is illegal though.
            if (a->note_on_state[channel])
                break; // busy
            mixer = a->mixer[channel];
            tone_switch = (mixer >> 5) & 1;
            noise_switch = (mixer >> 6) & 1;
            env_switch = (mixer >> 7) & 1;
            ayumi_set_mixer(a->impl, channel, tone_switch, noise_switch, env_switch);
            ayumi_set_tone(a->impl, channel, 2000000.0 / (16.0 * key_to_freq(msg[1])));
            a->note_on_state[channel] = true;
            break;
        case CMIDI2_STATUS_PROGRAM:
            noise = msg[1] & 0x1F;
            ayumi_set_noise(a->impl, noise);
            mixer = msg[1];
            tone_switch = (mixer >> 5) & 1;
            noise_switch = (mixer >> 6) & 1;
            // We cannot pass 8 bit message, so we remove env_switch here. Use BankMSB for it.
            env_switch = (a->mixer[channel] >> 7) & 1;
            a->mixer[channel] = msg[1];
            ayumi_set_mixer(a->impl, channel, tone_switch, noise_switch, env_switch);
            break;
        case CMIDI2_STATUS_CC:
            switch (msg[1]) {
                case CMIDI2_CC_BANK_SELECT:
                    mixer = msg[1];
                    tone_switch = mixer & 1;
                    noise_switch = (mixer >> 1) & 1;
                    env_switch = (mixer >> 2) & 1;
                    a->mixer[channel] = msg[1];
                    ayumi_set_mixer(a->impl, channel, tone_switch, noise_switch, env_switch);
                    break;
                case CMIDI2_CC_PAN:
                    ayumi_set_pan(a->impl, channel, msg[2] / 128.0, 0);
                    break;
                case CMIDI2_CC_VOLUME:
                    ayumi_set_volume(a->impl, channel, (msg[2] > 119 ? 119 : msg[2]) / 8); // FIXME: max is 14?? 15 doesn't work
                    break;
                case AYUMI_AAP_MIDI_CC_ENVELOPE_H:
                    a->envelope = (a->envelope & 0x3FFF) + (msg[2] << 14);
                    ayumi_set_envelope(a->impl, a->envelope);
                    break;
                case AYUMI_AAP_MIDI_CC_ENVELOPE_M:
                    a->envelope = (a->envelope & 0xC07F) + (msg[2] << 7);
                    ayumi_set_envelope(a->impl, a->envelope);
                    break;
                case AYUMI_AAP_MIDI_CC_ENVELOPE_L:
                    a->envelope = (a->envelope & 0xFF80) + msg[2];
                    ayumi_set_envelope(a->impl, a->envelope);
                    break;
                case AYUMI_AAP_MIDI_CC_ENVELOPE_SHAPE:
                    ayumi_set_envelope_shape(a->impl, msg[2] & 0xF);
                    break;
                case AYUMI_AAP_MIDI_CC_DC:
                    ayumi_remove_dc(a->impl);
                    break;
            }
            break;
        case CMIDI2_STATUS_PITCH_BEND:
            a->pitchbend = (msg[1] << 7) + msg[2];
            break;
        default:
            break;
    }
}



void sample_plugin_process(AndroidAudioPlugin *plugin,
                           AndroidAudioPluginBuffer *buffer,
                           long timeoutInNanoseconds) {

    auto context = (AyumiHandle*) plugin->plugin_specific;
    if (!context->active)
        return;

    auto aapmb = (AAPMidiBufferHeader*) buffer->buffers[AYUMI_AAP_MIDI2_IN_PORT];
    uint32_t lengthUnit = aapmb->time_options;

    int currentFrame = 0;

    auto outL = (float*) buffer->buffers[AYUMI_AAP_AUDIO_OUT_LEFT];
    auto outR = (float*) buffer->buffers[AYUMI_AAP_AUDIO_OUT_RIGHT];

    if (context->midi_protocol == AAP_PROTOCOL_MIDI2_0) {
        auto midi2ptr = ((uint32_t*) (void*) aapmb) + 8;
        CMIDI2_UMP_SEQUENCE_FOREACH(midi2ptr, aapmb->length, ev) {
            auto ump = (cmidi2_ump *) ev;
            if (cmidi2_ump_get_message_type(ump) == CMIDI2_MESSAGE_TYPE_UTILITY &&
                cmidi2_ump_get_status_code(ump) == CMIDI2_JR_TIMESTAMP) {
                int max = currentFrame + cmidi2_ump_get_jr_timestamp_timestamp(ump);
                max = max < buffer->num_frames ? max : buffer->num_frames;
                for (int i = currentFrame; i < max; i++) {
                    ayumi_process(context->impl);
                    ayumi_remove_dc(context->impl);
                    outL[i] = (float) context->impl->left;
                    outR[i] = (float) context->impl->right;
                }
                currentFrame = max;
            }
            ayumi_aap_process_midi_event(context, ev);
        }
    } else {
        auto midi1ptr = ((uint8_t*) (void*) aapmb) + 8;
        auto midi1end = midi1ptr + aapmb->length;
        while (midi1ptr < midi1end) {
            // get delta time
            uint32_t deltaTime = 0;
            for (int digits = 0;; digits++) {
                deltaTime += (0x7F & (*midi1ptr)) << (digits * 7);
                if (*midi1ptr < 0x80)
                    break;
                midi1ptr++;
            }
            midi1ptr++;

            // process audio until current time (max)
            int max = currentFrame + deltaTime;
            max = max < buffer->num_frames ? max : buffer->num_frames;
            for (int i = currentFrame; i < max; i++) {
                ayumi_process(context->impl);
                ayumi_remove_dc(context->impl);
                outL[i] = (float) context->impl->left;
                outR[i] = (float) context->impl->right;
            }
            currentFrame = max;

            // process next MIDI event
            ayumi_aap_process_midi_event(context, midi1ptr);
        }
    }

    for (int i = currentFrame; i < buffer->num_frames; i++) {
        ayumi_process(context->impl);
        ayumi_remove_dc(context->impl);
        outL[i] = (float) context->impl->left;
        outR[i] = (float) context->impl->right;
    }
}

void sample_plugin_deactivate(AndroidAudioPlugin *plugin) {
    auto context = (AyumiHandle*) plugin->plugin_specific;
    context->active = false;
}

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

    auto handle = new AyumiHandle();
    handle->active = false;
    handle->impl = (struct ayumi*) calloc(sizeof(struct ayumi), 1);
    handle->sample_rate = sampleRate;

    /* clock_rate / (sample_rate * 8 * 8) must be < 1.0 */
    ayumi_configure(handle->impl, 1, 2000000, (int) sampleRate);
    ayumi_set_noise(handle->impl, 4); // pink noise by default
    for (int i = 0; i < 3; i++) {
        handle->mixer[i] = 1 << 6; // tone, without envelope
        ayumi_set_pan(handle->impl, i, 0.5, 0); // 0(L)...1(R)
        ayumi_set_mixer(handle->impl, i, 1, 1, 0); // should be quiet by default
        ayumi_set_envelope_shape(handle->impl, 14); // see http://fmpdoc.fmp.jp/%E3%82%A8%E3%83%B3%E3%83%99%E3%83%AD%E3%83%BC%E3%83%97%E3%83%8F%E3%83%BC%E3%83%89%E3%82%A6%E3%82%A7%E3%82%A2/
        ayumi_set_envelope(handle->impl, 0x40); // somewhat slow
        ayumi_set_volume(handle->impl, i, 14); // FIXME: max = 14?? 15 doesn't work
    }

    for (int i = 0; extensions[i] != nullptr; i++) {
        AndroidAudioPluginExtension *ext = extensions[i];
        if (strcmp(AAP_MIDI_CI_EXTENSION_URI, ext->uri) == 0) {
            auto data = (MidiCIExtension*) ext->data;
            handle->midi_protocol = data->protocol == 2 ? AAP_PROTOCOL_MIDI2_0 : AAP_PROTOCOL_MIDI1_0;
        }
    }

    return new AndroidAudioPlugin{
            handle,
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
