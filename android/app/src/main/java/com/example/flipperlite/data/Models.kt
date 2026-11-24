package com.example.flipperlite.data

import org.json.JSONObject

data class DeviceStatus(
    val name: String,
    val version: String,
    val app: String,
    val mode: String,
    val ssid: String,
    val ip: String,
    val uptimeS: Long,
    val freeHeap: Long,
) {
    companion object {
        fun fromJson(json: String): DeviceStatus {
            val object = JSONObject(json)
            return DeviceStatus(
                name = object.optString("name", "unknown"),
                version = object.optString("version", "-"),
                app = object.optString("app", "none"),
                mode = object.optString("mode", "-"),
                ssid = object.optString("ssid", "-"),
                ip = object.optString("ip", "-"),
                uptimeS = object.optLong("uptime_s"),
                freeHeap = object.optLong("free_heap"),
            )
        }
    }
}

data class AppInfo(val id: String, val name: String)
