package com.example.flipperlite.data

import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import okhttp3.Call
import okhttp3.Callback
import okhttp3.MediaType.Companion.toMediaType
import okhttp3.OkHttpClient
import okhttp3.Request
import okhttp3.RequestBody.Companion.toRequestBody
import okhttp3.Response
import org.json.JSONArray
import org.json.JSONObject
import java.io.EOFException
import java.io.IOException
import java.util.concurrent.TimeUnit

/** HTTP client for the Go bridge. All calls are suspend functions except the SSE stream. */
class BridgeClient(url: String) {
    private val baseUrl = normalize(url)
    private val http = OkHttpClient.Builder()
        .connectTimeout(3, TimeUnit.SECONDS)
        .readTimeout(5, TimeUnit.SECONDS)
        .build()
    private val streamClient = http.newBuilder()
        .readTimeout(0, TimeUnit.MILLISECONDS)
        .build()

    suspend fun status(): DeviceStatus = withContext(Dispatchers.IO) {
        DeviceStatus.fromJson(get("/api/v1/status"))
    }

    suspend fun apps(): List<AppInfo> = withContext(Dispatchers.IO) {
        val array = JSONObject(get("/api/v1/apps")).getJSONArray("apps")
        (0 until array.length()).map { index ->
            val item = array.getJSONObject(index)
            AppInfo(item.getString("id"), item.getString("name"))
        }
    }

    suspend fun input(key: String, action: String) = withContext(Dispatchers.IO) {
        post("/api/v1/input", JSONObject().put("key", key).put("action", action))
    }

    suspend fun launch(id: String) = withContext(Dispatchers.IO) {
        post("/api/v1/launch", JSONObject().put("id", id))
    }

    /** Opens an SSE stream; every `data:` payload is delivered to [onLine]. */
    fun streamEvents(onLine: (String) -> Unit): Call {
        val call = streamClient.newCall(Request.Builder().url(baseUrl + "/api/v1/events").build())
        call.enqueue(object : Callback {
            override fun onFailure(call: Call, e: IOException) {
                onLine("stream error: ${e.message}")
            }

            override fun onResponse(call: Call, response: Response) {
                response.use { resp ->
                    if (!resp.isSuccessful) {
                        onLine("stream HTTP ${resp.code}")
                        return@use
                    }
                    val source = resp.body?.source() ?: return@use
                    try {
                        while (!call.isCanceled()) {
                            val line = source.readUtf8LineStrict() ?: break
                            if (line.startsWith("data: ")) onLine(line.removePrefix("data: "))
                        }
                    } catch (_: EOFException) {
                        // Device closed the stream; caller can restart it.
                    } catch (e: IOException) {
                        onLine("stream error: ${e.message}")
                    }
                }
            }
        })
        return call
    }

    private fun get(path: String): String {
        http.newCall(Request.Builder().url(baseUrl + path).build()).execute().use { response ->
            if (!response.isSuccessful) throw IOException("HTTP ${response.code} on $path")
            return response.body?.string() ?: throw IOException("empty response")
        }
    }

    private fun post(path: String, payload: JSONObject) {
        val body = payload.toString().toRequestBody("application/json".toMediaType())
        http.newCall(Request.Builder().url(baseUrl + path).post(body).build()).execute().use { response ->
            if (!response.isSuccessful) throw IOException("HTTP ${response.code} on $path")
        }
    }

    companion object {
        fun normalize(url: String): String {
            val trimmed = url.trim().trimEnd('/')
            return if (trimmed.startsWith("http://") || trimmed.startsWith("https://")) trimmed
            else "http://$trimmed"
        }
    }
}
