package com.example.flipperlite.ui

import android.os.Handler
import android.os.Looper
import android.util.Base64
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.horizontalScroll
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Button
import androidx.compose.material3.Card
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.material3.TopAppBar
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.input.PasswordVisualTransformation
import androidx.compose.ui.unit.dp
import com.example.flipperlite.data.AppInfo
import com.example.flipperlite.data.BridgeClient
import com.example.flipperlite.data.DeviceStatus
import com.example.flipperlite.data.DisplayFrame
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch

@Composable
fun DisplayPreview(frame: DisplayFrame?) {
    val width = frame?.width?.takeIf { it > 0 } ?: 128
    val height = frame?.height?.takeIf { it > 0 } ?: 64
    val pixels = remember(frame) {
        if (frame?.data.isNullOrEmpty()) ByteArray(width * height / 8)
        else Base64.decode(frame.data, Base64.DEFAULT)
    }
    Canvas(
        Modifier
            .fillMaxWidth()
            .height(140.dp)
            .background(Color(0xFF05080F)),
    ) {
        val scale = minOf(size.width / width, size.height / height)
        for (y in 0 until height) {
            for (x in 0 until width) {
                val index = (y / 8) * width + x
                if (index < pixels.size && (pixels[index].toInt() and (1 shl (y % 8))) != 0) {
                    drawRect(
                        color = Color(0xFF4ADE80),
                        topLeft = Offset(x * scale, y * scale),
                        size = Size(scale + 0.5f, scale + 0.5f),
                    )
                }
            }
        }
    }
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun AppScreen() {
    var url by rememberSaveable { mutableStateOf("http://10.0.2.2:8080") }
    var token by rememberSaveable { mutableStateOf("") }
    var wifiSsid by rememberSaveable { mutableStateOf("") }
    var wifiPassword by rememberSaveable { mutableStateOf("") }
    var wifiResult by remember { mutableStateOf("") }
    var status by remember { mutableStateOf<DeviceStatus?>(null) }
    var apps by remember { mutableStateOf<List<AppInfo>>(emptyList()) }
    var frame by remember { mutableStateOf<DisplayFrame?>(null) }
    var events by remember { mutableStateOf<List<String>>(emptyList()) }
    var error by remember { mutableStateOf<String?>(null) }
    val client = remember(url, token) { BridgeClient(url, token) }
    val scope = rememberCoroutineScope()
    val mainHandler = remember { Handler(Looper.getMainLooper()) }

    fun addEvent(line: String) {
        events = (events + line).takeLast(30)
    }

    LaunchedEffect(url) {
        while (true) {
            try {
                status = client.status()
                apps = client.apps()
                frame = client.display()
                error = null
            } catch (e: Exception) {
                error = e.message ?: "bridge unreachable"
            }
            delay(3000)
        }
    }

    DisposableEffect(url) {
        val call = client.streamEvents { line ->
            mainHandler.post {
                addEvent(line.removePrefix("{\"type\":").take(160))
            }
        }
        onDispose { call.cancel() }
    }

    Scaffold(topBar = { TopAppBar(title = { Text("FlipperLite") }) }) { padding ->
        Column(
            modifier = Modifier
                .padding(padding)
                .padding(16.dp)
                .verticalScroll(rememberScrollState()),
            verticalArrangement = Arrangement.spacedBy(12.dp),
        ) {
            OutlinedTextField(
                value = url,
                onValueChange = { url = it },
                label = { Text("Bridge URL") },
                singleLine = true,
                modifier = Modifier.fillMaxWidth(),
            )
            OutlinedTextField(
                value = token,
                onValueChange = { token = it },
                label = { Text("API token (optional)") },
                singleLine = true,
                visualTransformation = PasswordVisualTransformation(),
                modifier = Modifier.fillMaxWidth(),
            )
            if (error != null) {
                Text("Bridge offline: $error", color = MaterialTheme.colorScheme.error)
            }

            Text("WiFi sozlash (faqat o'z tarmog'ingiz)", style = MaterialTheme.typography.titleMedium)
            OutlinedTextField(
                value = wifiSsid,
                onValueChange = { wifiSsid = it },
                label = { Text("SSID") },
                singleLine = true,
                modifier = Modifier.fillMaxWidth(),
            )
            OutlinedTextField(
                value = wifiPassword,
                onValueChange = { wifiPassword = it },
                label = { Text("Parol") },
                singleLine = true,
                visualTransformation = PasswordVisualTransformation(),
                modifier = Modifier.fillMaxWidth(),
            )
            Row(verticalAlignment = Alignment.CenterVertically) {
                Button(onClick = {
                    scope.launch {
                        wifiResult = try {
                            client.connectWiFi(wifiSsid, wifiPassword)
                            wifiPassword = ""
                            "So'rov yuborildi"
                        } catch (e: Exception) { e.message ?: "xato" }
                    }
                }) { Text("Ulanish") }
                if (wifiResult.isNotEmpty()) {
                    Text(wifiResult, color = MaterialTheme.colorScheme.secondary)
                }
            }

            status?.let { current ->
                Card(modifier = Modifier.fillMaxWidth()) {
                    Column(Modifier.padding(12.dp)) {
                        Text("${current.name} v${current.version}", style = MaterialTheme.typography.titleMedium)
                        Text("App: ${current.app}   Mode: ${current.mode}")
                        Text("SSID: ${current.ssid}")
                        Text("IP: ${current.ip}")
                        Text("Uptime: ${current.uptimeS}s   Heap: ${current.freeHeap}")
                    }
                }
            }

            Text("Apps", style = MaterialTheme.typography.titleMedium)
            Row(Modifier.horizontalScroll(rememberScrollState())) {
                apps.forEach { app ->
                    Button(
                        onClick = {
                            scope.launch {
                                try { client.launch(app.id) } catch (e: Exception) { error = e.message }
                            }
                        },
                        modifier = Modifier.padding(end = 8.dp),
                    ) { Text(app.name) }
                }
            }

            Text("Displey", style = MaterialTheme.typography.titleMedium)
            DisplayPreview(frame)

            Text("D-pad", style = MaterialTheme.typography.titleMedium)
            fun send(key: String, action: String = "short") {
                scope.launch {
                    try { client.input(key, action) } catch (e: Exception) { error = e.message }
                }
            }
            Row(Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.Center) {
                Button(onClick = { send("up") }) { Text("UP") }
            }
            Row(Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.Center) {
                Button(onClick = { send("left") }) { Text("LEFT") }
                Spacer(Modifier.width(8.dp))
                Button(onClick = { send("ok") }) { Text("OK") }
                Spacer(Modifier.width(8.dp))
                Button(onClick = { send("right") }) { Text("RIGHT") }
            }
            Row(Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.Center) {
                Button(onClick = { send("down") }) { Text("DOWN") }
            }
            Row(Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.Center) {
                Button(onClick = { send("back") }) { Text("BACK") }
                Spacer(Modifier.width(8.dp))
                Button(onClick = { send("ok", "long") }) { Text("OK LONG") }
            }

            Text("Events", style = MaterialTheme.typography.titleMedium)
            events.takeLast(8).forEach { Text(it, style = MaterialTheme.typography.bodySmall) }
            Spacer(Modifier.height(16.dp))
        }
    }
}
