
#ifndef AAP_MIDI_DEVICE_SERVICE_AAPMIDIPROCESSOR_H
#define AAP_MIDI_DEVICE_SERVICE_AAPMIDIPROCESSOR_H

#include <oboe/Oboe.h>
#include <zix/ring.h>
#include <cmidi2.h>

namespace aapmidideviceservice {

    enum AAPMidiProcessorState {
        AAP_MIDI_PROCESSOR_STATE_CREATED,
        AAP_MIDI_PROCESSOR_STATE_ACTIVE,
        AAP_MIDI_PROCESSOR_STATE_INACTIVE,
        AAP_MIDI_PROCESSOR_STATE_STOPPED,
        AAP_MIDI_PROCESSOR_STATE_ERROR
    };

    class PluginInstanceData {
        std::vector<int> audio_out_ports{};
        std::vector<int> port_shared_memory_fds{};

    public:
        PluginInstanceData(int instanceId, size_t numPorts) : instance_id(instanceId), num_ports(numPorts) {
            auto arr = (void**) calloc(sizeof(void*), numPorts + 1);
            arr[numPorts] = nullptr;
            buffer_pointers.reset(arr);
        }
        inline std::vector<int32_t>* getAudioOutPorts() { return &audio_out_ports; }
        inline int32_t getPortSharedMemoryFD(size_t index) { return port_shared_memory_fds[index]; }
        inline void setPortSharedMemoryFD(size_t index, int32_t fd) {
            if (port_shared_memory_fds.size() <= index)
                port_shared_memory_fds.resize(index + 1);
            port_shared_memory_fds[index] = fd;
        }

        int instance_id;
        int num_ports;
        int midi1_in_port{-1};
        int midi2_in_port{-1};
        std::unique_ptr<AndroidAudioPluginBuffer> plugin_buffer;
        std::unique_ptr<void*> buffer_pointers{nullptr};
    };

    class AAPMidiProcessor {

        class OboeCallback : public oboe::AudioStreamDataCallback {
        public:
            OboeCallback(AAPMidiProcessor *owner) : owner(owner) {}

            AAPMidiProcessor *owner;
            oboe::DataCallbackResult
            onAudioReady(oboe::AudioStream *audioStream, void *audioData, int32_t numFrames);
        };

        static std::string convertStateToText(AAPMidiProcessorState state);

        // AAP
        aap::PluginHostManager host_manager{};
        std::unique_ptr<aap::PluginHost> host{nullptr};
        int sample_rate{0};
        int aap_frame_size{4096};
        int channel_count{2};
        std::vector<std::unique_ptr<PluginInstanceData>> instance_data_list{};
        int instrument_instance_id{0};
        int32_t midi_protocol{CMIDI2_PROTOCOL_TYPE_MIDI1};

        PluginInstanceData* getAAPMidiInputData();
        void* getAAPMidiInputBuffer();

        // Oboe
        oboe::AudioStreamBuilder builder{};
        std::unique_ptr<oboe::AudioStreamDataCallback> callback{};
        std::shared_ptr<oboe::AudioStream> stream{};
        AAPMidiProcessorState state{AAP_MIDI_PROCESSOR_STATE_CREATED};

        ZixRing *aap_input_ring_buffer{nullptr};
        float *interleave_buffer{nullptr};
        struct timespec last_aap_process_time{};
    public:
        static AAPMidiProcessor* getInstance();
        static void resetInstance();

        void initialize(int32_t sampleRate, int32_t oboeBurstFrameSize, int32_t channelCount, int32_t aapFrameSize);

        static void registerPluginService(std::unique_ptr<aap::AudioPluginServiceConnection> service);

        void instantiatePlugin(std::string pluginId);

        void activate();

        void processMidiInput(uint8_t* bytes, size_t offset, size_t length, int64_t timestampInNanoseconds);

        void setMidiProtocol(int32_t midiProtocol);

        void callPluginProcess();

        void fillAudioOutput();

        void deactivate();

        void terminate();
    };
}

#endif //AAP_MIDI_DEVICE_SERVICE_AAPMIDIPROCESSOR_H
