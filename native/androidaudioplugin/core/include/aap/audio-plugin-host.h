#pragma once

#ifndef _ANDROID_AUDIO_PLUGIN_HOST_HPP_
#define _ANDROID_AUDIO_PLUGIN_HOST_HPP_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <dlfcn.h>
#include <time.h>
#include <unistd.h>
#include <memory>
#include <vector>
#include <map>
#include <string>
#include "aap/android-audio-plugin.h"
#include "aap/parameter-properties.h"

namespace aap {

void set_application_context(void* javaVM, void* jobjectApplicationContext);

enum ContentType {
	AAP_CONTENT_TYPE_UNDEFINED = 0,
	AAP_CONTENT_TYPE_AUDIO = 1,
	AAP_CONTENT_TYPE_MIDI = 2,
	AAP_CONTENT_TYPE_MIDI2 = 3,
};

enum PortDirection {
	AAP_PORT_DIRECTION_INPUT,
	AAP_PORT_DIRECTION_OUTPUT
};

enum PluginInstantiationState {
	PLUGIN_INSTANTIATION_STATE_INITIAL,
	PLUGIN_INSTANTIATION_STATE_UNPREPARED,
	PLUGIN_INSTANTIATION_STATE_INACTIVE,
	PLUGIN_INSTANTIATION_STATE_ACTIVE,
	PLUGIN_INSTANTIATION_STATE_TERMINATED,
};

class PropertyContainer {
	std::map<std::string, std::string> properties{};

protected:
	PropertyContainer() {}

	// deprecated
	bool hasValueRange() const { return hasProperty(AAP_PARAMETER_DEFAULT); }

	// deprecated
	float getDefaultValue() const { return hasProperty(AAP_PARAMETER_DEFAULT) ? getPropertyAsFloat(AAP_PARAMETER_DEFAULT) : 0.0f; }
	// deprecated
	float getMinimumValue() const { return hasProperty(AAP_PARAMETER_MINIMUM) ? getPropertyAsFloat(AAP_PARAMETER_MINIMUM) : 0.0f; }
	// deprecated
	float getMaximumValue() const { return hasProperty(AAP_PARAMETER_MAXIMUM) ? getPropertyAsFloat(AAP_PARAMETER_MAXIMUM) : 0.0f; }

public:

	bool getPropertyAsBoolean(std::string id) const {
		return getPropertyAsDouble(id) > 0;
	}
	int getPropertyAsInteger(std::string id) const {
		return atoi(getPropertyAsString(id).c_str());
	}
	float getPropertyAsFloat(std::string id) const {
		return (float) atof(getPropertyAsString(id).c_str());
	}
	double getPropertyAsDouble(std::string id) const {
		return atof(getPropertyAsString(id).c_str());
	}
	bool hasProperty(std::string id) const {
		return properties.find(id) != properties.end();
	}
	std::string getPropertyAsString(std::string id) const {
		return properties.find(id)->second;
	}

	void setPropertyValueString(std::string id, std::string value) {
		properties[id] = value;
	}
};

class ParameterInformation : public PropertyContainer {
	std::string name{};
	ContentType content_type;

public:
	ParameterInformation(std::string parameterName, ContentType content)
	: name(parameterName), content_type(content)
	{
	}

	const char* getName() const { return name.c_str(); }
	ContentType getContentType() const { return content_type; }
};

class PortInformation : public PropertyContainer
{
	std::string name{};
	ContentType content_type;
	PortDirection direction;
	std::map<std::string, std::string> properties{};
	
public:
	PortInformation(std::string portName, ContentType content, PortDirection portDirection)
			: name(portName), content_type(content), direction(portDirection)
	{
	}

	const char* getName() const { return name.c_str(); }
	ContentType getContentType() const { return content_type; }
	PortDirection getPortDirection() const { return direction; }
};

class PluginInformation
{
	// hosting information
	bool is_out_process;

	// basic information
	std::string plugin_package_name{}; // service package name for Android, FIXME: determine semantics for desktop
	std::string plugin_local_name{}; // service class name for Android, FIXME: determine semantics for desktop
	std::string display_name{};
	std::string manufacturer_name{};
	std::string version{};
	std::string identifier_string{};
	std::string shared_library_filename{};
	std::string library_entrypoint{};
	std::string plugin_id{};
	std::string metadata_full_path{};
	int64_t last_info_updated_unixtime_milliseconds;

	/* NULL-terminated list of categories, separate by | */
	std::string primary_category{};
	/* NULL-terminated list of ports */
	std::vector<const PortInformation*> ports;
	/* NULL-terminated list of ports */
	std::vector<const ParameterInformation*> parameters;
	/* NULL-terminated list of required extensions */
	std::vector<std::unique_ptr<std::string>> required_extensions;
	/* NULL-terminated list of optional extensions */
	std::vector<std::unique_ptr<std::string>> optional_extensions;

public:

	/* In VST3 world, they are like "Effect", "Instrument", "Instrument|Synth", "Fx|Delay" ... can be anything. Here we list typical-looking ones */
	const char * PRIMARY_CATEGORY_EFFECT = "Effect";
	const char * PRIMARY_CATEGORY_INSTRUMENT = "Instrument";

	PluginInformation(bool isOutProcess, std::string pluginPackageName, std::string pluginLocalName, std::string displayName, std::string manufacturerName, std::string versionString, std::string pluginID, std::string sharedLibraryFilename, std::string libraryEntrypoint, std::string metadataFullPath, std::string primaryCategory);

	~PluginInformation()
	{
	}

	const std::string getPluginPackageName() const
	{
		return plugin_package_name;
	}

	const std::string getPluginLocalName() const
	{
		return plugin_local_name;
	}

	const std::string getDisplayName() const
	{
		return display_name;
	}
	
	const std::string getManufacturerName() const
	{
		return manufacturer_name;
	}
	
	const std::string getVersion() const
	{
		return version;
	}
	
	/* locally identifiable string.
	 * It is combination of name+unique_id+version, to support plugin debugging. */
	const std::string getStrictIdentifier() const
	{
		return identifier_string;
	}
	
	const std::string getPrimaryCategory() const
	{
		return primary_category;
	}
	
	int getNumPorts() const
	{
		return (int) ports.size();
	}
	
	const PortInformation* getPort(int index) const
	{
		return ports[(size_t) index];
	}

	int getNumParameters() const
	{
		return (int) parameters.size();
	}

	const ParameterInformation* getParameter(int index) const
	{
		return parameters[(size_t) index];
	}

	int getNumRequiredExtensions() const
	{
		return (int) required_extensions.size();
	}
	
	const std::string* getRequiredExtension(int index) const
	{
		return required_extensions[(size_t) index].get();
	}
	
	int getNumOptionalExtensions() const
	{
		return (int) optional_extensions.size();
	}
	
	const std::string* getOptionalExtension(int index) const
	{
		return optional_extensions[(size_t) index].get();
	}
	
	int64_t getLastInfoUpdateTime() const
	{
		return last_info_updated_unixtime_milliseconds;
	}
	
	/* unique identifier across various environment */
	const std::string getPluginID() const
	{
		return plugin_id;
	}

	const std::string getMetadataFullPath() const
	{
		return metadata_full_path;
	}

	bool isInstrument() const
	{
		// The purpose of this function seems to be based on hacky premise. So do we.
		return strstr(getPrimaryCategory().c_str(), "Instrument") != nullptr;
	}
	
	bool hasSharedContainer() const
	{
		// TODO: FUTURE (v0.8) It may be something AAP should support because
		// context switching over outprocess plugins can be quite annoying...
		return false;
	}
	
	bool hasEditor() const
	{
		// TODO: FUTURE (v0.7)
		return false;
	}

	const std::string getLocalPluginSharedLibrary() const
	{
		// By metadata or inferred.
		// Since Android expects libraries stored in `lib` directory,
		// it will just return the name of the shared library.
		return shared_library_filename;
	}

	const std::string getLocalPluginLibraryEntryPoint() const
	{
		// can be null. "GetAndroidAudioPluginFactory" is used by default.
		return library_entrypoint;
	}
	
	bool isOutProcess() const
	{
		return is_out_process;
	}

	void addParameter(ParameterInformation* para)
	{
		parameters.push_back(para);
	}

	void addPort(PortInformation* port)
	{
		ports.push_back(port);
	}
};


class PluginHostPAL
{
protected:
	// FIXME: move to PluginHost
	std::vector<PluginInformation*> plugin_list_cache{};

public:
	~PluginHostPAL() {
		for (auto plugin : plugin_list_cache)
			delete plugin;
	}

	virtual std::string getRemotePluginEntrypoint() = 0;
	virtual std::vector<std::string> getPluginPaths() = 0;
	virtual void getAAPMetadataPaths(std::string path, std::vector<std::string>& results) = 0;
	virtual std::vector<PluginInformation*> getPluginsFromMetadataPaths(std::vector<std::string>& aapMetadataPaths) = 0;

	// FIXME: move to PluginHost
	std::vector<PluginInformation*> getPluginListCache() { return plugin_list_cache; }

	// FIXME: move to PluginHost
    void setPluginListCache(std::vector<PluginInformation*> pluginInfos) {
    	plugin_list_cache.clear();
    	for (auto p : pluginInfos)
    		plugin_list_cache.emplace_back(p);
    }

	std::vector<PluginInformation*> getInstalledPlugins(bool returnCacheIfExists = true, std::vector<std::string>* searchPaths = nullptr);
};

PluginHostPAL* getPluginHostPAL();

#define AAP_SHARED_MEMORY_EXTENSION_URI "aap-extension:org.androidaudioplugin.SharedMemoryExtension"

class SharedMemoryExtension
{
	std::unique_ptr<std::vector<int32_t>> port_buffer_fds{nullptr};
	std::unique_ptr<std::vector<int32_t>> extension_fds{nullptr};

public:
    SharedMemoryExtension();
    ~SharedMemoryExtension() {
        for (int32_t fd : *port_buffer_fds)
        	if (fd)
	            close(fd);
        port_buffer_fds->clear();
		for (int32_t fd : *extension_fds)
			if (fd)
				close(fd);
		extension_fds->clear();
    }

    // Stores clone of port buffer FDs passed from client via Binder.
    inline void resizePortBuffer(size_t newSize) { port_buffer_fds->resize(newSize); }
    inline int32_t getPortBufferFD(size_t index) { return port_buffer_fds->at(index); }
    inline void setPortBufferFD(size_t index, int32_t fd) {
    	if (port_buffer_fds->size() <= index)
    		port_buffer_fds->resize(index + 1);
    	port_buffer_fds->at(index) = fd;
    }
    // Stores clone of extension data FDs passed from client via Binder.
	inline std::vector<int32_t>* getExtensionFDs() { return extension_fds.get(); }
};



class PluginHostManager
{
	std::vector<const PluginInformation*> plugin_infos{};

public:

	PluginHostManager();

	~PluginHostManager()
	{
	}

	void updateKnownPlugins(std::vector<PluginInformation*> plugins);

	bool isPluginAlive (std::string identifier);

	bool isPluginUpToDate (std::string identifier, int64_t lastInfoUpdated);

	size_t getNumPluginInformation()
	{
		return plugin_infos.size();
	}

	const PluginInformation* getPluginInformation(int32_t index)
	{
		return plugin_infos[(size_t) index];
	}

	const PluginInformation* getPluginInformation(std::string identifier)
	{
		size_t n = getNumPluginInformation();
		for(size_t i = 0; i < n; i++) {
			auto d = getPluginInformation(i);
			if (d->getPluginID().compare(identifier) == 0)
				return d;
		}
		return nullptr;
	}
};


class PluginInstance;

class PluginHost
{
	std::vector<PluginInstance*> instances{};
	PluginHostManager* manager{nullptr};

	PluginInstance* instantiateLocalPlugin(const PluginInformation *pluginInfo, int sampleRate);
	PluginInstance* instantiateRemotePlugin(const PluginInformation *pluginInfo, int sampleRate);

public:
	PluginHost(PluginHostManager* pluginHostManager)
			: manager(pluginHostManager)
	{
	}

	int createInstance(std::string identifier, int sampleRate, bool isRemoteExplicit = false);
	void destroyInstance(PluginInstance* instance);
	size_t getInstanceCount() { return instances.size(); }
	PluginInstance* getInstance(int32_t instanceId) { return instances[(size_t) instanceId]; }
};


// This is persistable AndroidAudioPluginExtension equivalent that can be stored in other persistent objects.
class PluginExtension {
public:
	PluginExtension(AndroidAudioPluginExtension src);

    std::unique_ptr<char> uri{nullptr};
    int32_t dataSize;
    std::unique_ptr<uint8_t> data{nullptr}; // pointer to void does not work...

    AndroidAudioPluginExtension asTransient() const;
};

class PluginInstance
{
	friend class PluginHost;
	friend class PluginHostManager;
	
	int sample_rate{44100};
	const PluginInformation *pluginInfo;
	AndroidAudioPluginFactory *plugin_factory;
	AndroidAudioPlugin *plugin;
	std::vector<std::unique_ptr<PluginExtension>> extensions{};
	PluginInstantiationState instantiation_state;
	AndroidAudioPluginState plugin_state{0, nullptr};

	PluginInstance(PluginHost* pluginHost, const PluginInformation* pluginInformation, AndroidAudioPluginFactory* loadedPluginFactory, int sampleRate)
		: sample_rate(sampleRate),
		  pluginInfo(pluginInformation),
		  plugin_factory(loadedPluginFactory),
		  plugin(nullptr),
		  instantiation_state(PLUGIN_INSTANTIATION_STATE_INITIAL)
	{
		assert(pluginInformation);
		assert(loadedPluginFactory);
	}
	
public:

    ~PluginInstance() { dispose(); }

	const PluginInformation* getPluginInformation()
	{
		return pluginInfo;
	}

	void addExtension(AndroidAudioPluginExtension ext)
	{
		extensions.emplace_back(std::make_unique<PluginExtension>(ext));
	}

	void completeInstantiation()
	{
		assert(instantiation_state == PLUGIN_INSTANTIATION_STATE_INITIAL);

		AndroidAudioPluginExtension extArr[extensions.size()];
		for (size_t i = 0; i < extensions.size(); i++)
			extArr[i] = extensions[i]->asTransient();
		AndroidAudioPluginExtension* extPtrArr[extensions.size() + 1];
		for (size_t i = 0; i < extensions.size(); i++)
			extPtrArr[i] = &extArr[i];
		extPtrArr[extensions.size()] = nullptr;
		plugin = plugin_factory->instantiate(plugin_factory, pluginInfo->getPluginID().c_str(), sample_rate, extPtrArr);
		assert(plugin);

		instantiation_state = PLUGIN_INSTANTIATION_STATE_UNPREPARED;
	}

	const void* getExtension(const char* uri)
	{
		for (auto& ext : extensions)
			if (strcmp(ext->uri.get(), uri) == 0)
				return ext->data.get();
		return nullptr;
	}

	inline SharedMemoryExtension* getSharedMemoryExtension() { return (SharedMemoryExtension*) getExtension(AAP_SHARED_MEMORY_EXTENSION_URI); }

	void prepare(int maximumExpectedSamplesPerBlock, AndroidAudioPluginBuffer *preparedBuffer)
	{
		assert(instantiation_state == PLUGIN_INSTANTIATION_STATE_UNPREPARED || instantiation_state == PLUGIN_INSTANTIATION_STATE_INACTIVE);

		plugin->prepare(plugin, preparedBuffer);
		instantiation_state = PLUGIN_INSTANTIATION_STATE_INACTIVE;
	}
	
	void activate()
	{
		if (instantiation_state == PLUGIN_INSTANTIATION_STATE_ACTIVE)
			return;
		assert(instantiation_state == PLUGIN_INSTANTIATION_STATE_INACTIVE);
		
		plugin->activate(plugin);
		instantiation_state = PLUGIN_INSTANTIATION_STATE_ACTIVE;
	}
	
	void deactivate()
	{
		if (instantiation_state == PLUGIN_INSTANTIATION_STATE_INACTIVE)
			return;
		assert(instantiation_state == PLUGIN_INSTANTIATION_STATE_ACTIVE);
		
		plugin->deactivate(plugin);
		instantiation_state = PLUGIN_INSTANTIATION_STATE_INACTIVE;
	}
	
	void dispose()
	{
		if (plugin != nullptr)
			plugin_factory->release(plugin_factory, plugin);
		plugin = nullptr;
		instantiation_state = PLUGIN_INSTANTIATION_STATE_TERMINATED;
	}
	
	void process(AndroidAudioPluginBuffer *buffer, int32_t timeoutInNanoseconds)
	{
		// It is not a TODO here, but if pointers have changed, we have to reconnect
		// LV2 ports.
		plugin->process(plugin, buffer, timeoutInNanoseconds);
	}

	int getNumPrograms()
	{
		// TODO: FUTURE (v0.6). LADSPA does not support it either.
		return 0;
	}
	
	int getCurrentProgram()
	{
		// TODO: FUTURE (v0.6). LADSPA does not support it either.
		return 0;
	}
	
	void setCurrentProgram(int index)
	{
		// TODO: FUTURE (v0.6). LADSPA does not support it, but resets all parameters.
	}

	std::string getProgramName(int index)
	{
		// TODO: FUTURE (v0.6). LADSPA does not support it either.
		return nullptr;
	}
	
	void changeProgramName(int index, std::string newName)
	{
		// TODO: FUTURE (v0.6). LADSPA does not support it either.
	}

	size_t getStateSize()
	{
		AndroidAudioPluginState result{0, nullptr};
		plugin->get_state(plugin, &result);
		return result.data_size;
	}

	const AndroidAudioPluginState& getState()
	{
		AndroidAudioPluginState result{0, nullptr};
		plugin->get_state(plugin, &result);
		if (plugin_state.data_size < result.data_size) {
			if (plugin_state.raw_data != nullptr)
				free((void*) plugin_state.raw_data);
			plugin_state.raw_data = calloc(result.data_size, 1);
		}
		plugin_state.data_size = result.data_size;
		plugin->get_state(plugin, &plugin_state);

		return plugin_state;
	}
	
	void setState(const void* data, size_t sizeInBytes)
	{
		AndroidAudioPluginState state;
		state.data_size = sizeInBytes;
		state.raw_data = data;
		plugin->set_state(plugin, &state);
	}
	
	uint32_t getTailTimeInMilliseconds()
	{
		// TODO: FUTURE (v0.6) - most likely just a matter of plugin property
		return 0;
	}
};


} // namespace

#endif // _ANDROID_AUDIO_PLUGIN_HOST_HPP_

