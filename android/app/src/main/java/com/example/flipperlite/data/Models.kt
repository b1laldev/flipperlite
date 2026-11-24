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
            val jsonObject = JSONObject(json)
            return DeviceStatus(
                name = jsonObject.optString("name", "unknown"),
                version = jsonObject.optString("version", "-"),
                app = jsonObject.optString("app", "none"),
                mode = jsonObject.optString("mode", "-"),
                ssid = jsonObject.optString("ssid", "-"),
                ip = jsonObject.optString("ip", "-"),
                uptimeS = jsonObject.optLong("uptime_s"),
                freeHeap = jsonObject.optLong("free_heap"),
            )
        }
    }
}

data class AppInfo(val id: String, val name: String)

data class DisplayFrame(val width: Int, val height: Int, val data: String) {
    companion object {
        fun fromJson(json: String): DisplayFrame {
            val jsonObject = JSONObject(json)
            return DisplayFrame(
                width = jsonObject.optInt("width", 128),
                height = jsonObject.optInt("height", 64),
                data = jsonObject.optString("data", ""),
            )
        }
    }
}
