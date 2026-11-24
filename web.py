#!/usr/bin/env python3
"""Veb-dashboard: tugmali va ekranli qurilma maketi (faqat localhost)."""

import json
from functools import partial
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

from sim import new_badge

HTML_PAGE = """<!doctype html>
<html lang="uz">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>WiFi Badge — simulyator</title>
<style>
  :root { color-scheme: light; }
  * { box-sizing: border-box; }
  body {
    margin: 0; min-height: 100vh; display: grid; place-items: center;
    font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif;
    background: #eceff1; color: #263238;
  }
  .wrap { display: flex; gap: 36px; align-items: flex-start; flex-wrap: wrap; justify-content: center; padding: 28px; }
  .device {
    width: 330px; padding: 22px; border-radius: 26px;
    background: linear-gradient(160deg, #455a64, #263238);
    box-shadow: 0 18px 40px rgba(0,0,0,.35), inset 0 1px 0 rgba(255,255,255,.12);
  }
  .brand { color: #cfd8dc; font-size: 12px; letter-spacing: .18em; text-align: center; margin-bottom: 14px; }
  .screen {
    background: #dfe6d2; color: #24321f; border-radius: 8px; padding: 18px 14px;
    font-family: "SFMono-Regular", Consolas, monospace; font-size: 15px; line-height: 1.55;
    white-space: pre; min-height: 170px; box-shadow: inset 0 2px 10px rgba(0,0,0,.18);
    border: 1px solid #b9c3ad;
  }
  .bar { display: flex; align-items: center; justify-content: space-between; margin: 16px 2px 0; }
  .leds { display: flex; gap: 9px; align-items: center; color: #b0bec5; font-size: 11px; }
  .led { width: 11px; height: 11px; border-radius: 50%; background: #37474f; border: 1px solid #62727b; }
  .led.on { background: #ffca28; box-shadow: 0 0 12px #ffca28; }
  .state { color: #b0bec5; font: 12px monospace; }
  .controls { display: flex; gap: 12px; margin-top: 20px; }
  button {
    flex: 1; padding: 13px 8px; border: 0; border-radius: 12px; cursor: pointer;
    font-weight: 600; font-size: 14px; color: #fff; background: #546e7a;
  }
  button:hover { filter: brightness(1.12); }
  button#wps { background: #ef6c00; }
  .info {
    background: #fff; border-radius: 16px; padding: 22px; width: 300px;
    box-shadow: 0 8px 24px rgba(0,0,0,.08);
  }
  .info h1 { margin: 0 0 6px; font-size: 19px; }
  .info p { margin: 0 0 16px; color: #607d8b; font-size: 13px; line-height: 1.5; }
  .row { display: flex; justify-content: space-between; padding: 9px 0; border-top: 1px solid #eceff1; font-size: 13px; }
  .row span:last-child { font-family: monospace; color: #37474f; }
  .note { margin-top: 16px; font-size: 11px; color: #90a4ae; line-height: 1.5; }
</style>
</head>
<body>
<div class="wrap">
  <div class="device">
    <div class="brand">W I F I &nbsp; B A D G E &nbsp; v0.1</div>
    <div class="screen" id="screen">ishga tushyapti...</div>
    <div class="bar">
      <div class="leds"><span class="led" id="led"></span> WPS</div>
      <div class="state" id="state">BOOT</div>
    </div>
    <div class="controls">
      <button id="wps">WPS tugmasi</button>
      <button id="reset">Reset</button>
    </div>
  </div>
  <div class="info">
    <h1>Qurilma holati</h1>
    <p>Bu soxta router bilan ishlaydigan o'quv simulyatori. Haqiqiy WiFi tarmoqlari skanerlanmaydi.</p>
    <div class="row"><span>holat</span><span id="i-state">—</span></div>
    <div class="row"><span>SSID</span><span id="i-ssid">—</span></div>
    <div class="row"><span>WPS oynasi</span><span id="i-wps">—</span></div>
    <div class="row"><span>PIN</span><span id="i-pin">—</span></div>
    <div class="note">Ekran har 700 ms da yangilanadi. WPS oynasi 6 soniyadan keyin yopiladi — vaqt o'tib ketsa xato ko'rsatiladi.</div>
  </div>
</div>
<script>
function paint(s) {
  document.getElementById('screen').textContent =
    s.lines.map(function (l) { return l.replace(/\\s+$/, ''); }).join('\\n');
  document.getElementById('state').textContent = s.state;
  document.getElementById('led').className = s.wps_open ? 'led on' : 'led';
  document.getElementById('i-state').textContent = s.state;
  document.getElementById('i-ssid').textContent = s.ssid || '—';
  document.getElementById('i-wps').textContent = s.wps_open ? 'ochiq' : 'yopiq';
  document.getElementById('i-pin').textContent = s.pin || '—';
}
function post(path) {
  fetch(path, {method: 'POST'}).then(function (r) { return r.json(); }).then(paint);
}
fetch('/api/state').then(function (r) { return r.json(); }).then(paint);
setInterval(function () { post('/api/tick'); }, 700);
document.getElementById('wps').onclick = function () { post('/api/wps'); };
document.getElementById('reset').onclick = function () { post('/api/reset'); };
</script>
</body>
</html>
"""


class BadgeHandler(BaseHTTPRequestHandler):
    badge = None

    def _send_json(self, payload: dict, status: int = 200) -> None:
        body = json.dumps(payload, ensure_ascii=False).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self):
        if self.path == "/api/state":
            self._send_json(self.badge.snapshot())
        elif self.path in ("/", "/index.html"):
            body = HTML_PAGE.encode("utf-8")
            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)
        else:
            self._send_json({"error": "not found"}, status=404)

    def do_POST(self):
        if self.path == "/api/wps":
            self.badge.press_wps()
        elif self.path == "/api/reset":
            self.badge.press_reset()
        elif self.path == "/api/tick":
            self.badge.tick()
        else:
            self._send_json({"error": "not found"}, status=404)
            return
        self._send_json(self.badge.snapshot())

    def log_message(self, fmt, *args):
        pass  # chiqishni shovqinli qilmaslik uchun


def main() -> None:
    handler = partial(BadgeHandler, badge=new_badge())
    server = ThreadingHTTPServer(("127.0.0.1", 8765), handler)
    print("http://127.0.0.1:8765  (to'xtatish: Ctrl+C)")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nTo'xtatildi.")
    finally:
        server.server_close()


if __name__ == "__main__":
    main()
