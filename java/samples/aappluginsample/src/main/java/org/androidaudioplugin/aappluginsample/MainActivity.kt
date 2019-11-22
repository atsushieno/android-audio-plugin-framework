package org.androidaudioplugin.aappluginsample

import android.content.Context
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.ArrayAdapter
import androidx.appcompat.app.AppCompatActivity
import kotlinx.android.synthetic.main.activity_main.*
import kotlinx.android.synthetic.main.audio_plugin_service_list_item.view.*
import org.androidaudioplugin.AudioPluginHost
import org.androidaudioplugin.AudioPluginService
import org.androidaudioplugin.AudioPluginServiceInformation
import org.androidaudioplugin.PluginInformation

class MainActivity : AppCompatActivity() {

    inner class PluginViewAdapter(ctx:Context, layout: Int, isOutProcess: Boolean, array: Array<Pair<AudioPluginServiceInformation,PluginInformation>>)
        : ArrayAdapter<Pair<AudioPluginServiceInformation,PluginInformation>>(ctx, layout, array)
    {
        override fun getView(position: Int, convertView: View?, parent: ViewGroup): View {
            val item = getItem(position)
            var view = convertView
            if (view == null)
                view = LayoutInflater.from(context).inflate(R.layout.audio_plugin_service_list_item, parent, false)
            if (view == null)
                throw UnsupportedOperationException()
            if (item == null)
                return view
            view.audio_plugin_service_name.text = item.first.name
            view.audio_plugin_name.text = item.second.name
            view.audio_plugin_identifier.text = item.second.pluginId

            return view
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        AudioPluginService.enableDebug = true

        // Query AAPs
        val pluginServices = AudioPluginHost.queryAudioPluginServices(this)
        val servicedPlugins = pluginServices.flatMap { s -> s.plugins.map { p -> Pair(s, p) } }.toTypedArray()

        val localPlugins = servicedPlugins.filter { p -> p.first.packageName == applicationInfo.packageName && p.second.backend == "LV2" }.toTypedArray()
        val localAdapter = PluginViewAdapter(this, R.layout.audio_plugin_service_list_item, false, localPlugins)
        this.localAudioPluginListView.adapter = localAdapter
    }
}
