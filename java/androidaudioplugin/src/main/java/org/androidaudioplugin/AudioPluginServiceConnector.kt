package org.androidaudioplugin

import android.content.ComponentName
import android.content.Context
import android.content.Intent
import android.util.Log
import org.androidaudioplugin.AudioPluginHostHelper.Companion

class AudioPluginServiceConnector(private val applicationContext: Context) : AutoCloseable {
    val serviceConnectedListeners = mutableListOf<(conn: PluginServiceConnection) -> Unit>()
    val connectedServices = mutableListOf<PluginServiceConnection>()
    private var isClosed = false

    fun bindAudioPluginService(service: AudioPluginServiceInformation, sampleRate: Int) {
        val intent = Intent(AudioPluginHostHelper.AAP_ACTION_NAME)
        intent.component = ComponentName(
            service.packageName,
            service.className
        )
        intent.putExtra("sampleRate", sampleRate)

        val conn = PluginServiceConnection(service) { c -> onBindAudioPluginService(c) }

        Log.d(
            "AudioPluginHost",
            "bindAudioPluginService: ${service.packageName} | ${service.className}"
        )
        applicationContext.bindService(intent, conn, Context.BIND_AUTO_CREATE)
    }

    private fun onBindAudioPluginService(conn: PluginServiceConnection) {
        AudioPluginNatives.addBinderForHost(
            conn.serviceInfo.packageName,
            conn.serviceInfo.className,
            conn.binder!!
        )
        connectedServices.add(conn)

        // avoid conflicting concurrent updates
        val currentListeners = serviceConnectedListeners.toTypedArray()
        currentListeners.forEach { f -> f(conn) }
    }

    fun findExistingServiceConnection(packageName: String, className: String) =
        connectedServices.firstOrNull { conn -> conn.serviceInfo.packageName == packageName && conn.serviceInfo.className == className }

    fun unbindAudioPluginService(packageName: String, localName: String) {
        val conn = findExistingServiceConnection(packageName, localName) ?: return
        connectedServices.remove(conn)
        AudioPluginNatives.removeBinderForHost(
            conn.serviceInfo.packageName,
            conn.serviceInfo.className
        )
    }

    override fun close() {
        while (connectedServices.any()) {
            val list = connectedServices.toTypedArray()
            for (conn in list)
                AudioPluginNatives.removeBinderForHost(
                    conn.serviceInfo.packageName,
                    conn.serviceInfo.className
                )
        }
        serviceConnectedListeners.clear()
    }
}