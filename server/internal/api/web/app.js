(() => {
  'use strict';

  const state = {
    token: localStorage.getItem('flite.token') || '',
    connected: false,
  };

  const $ = (selector) => document.querySelector(selector);
  const sleep = (ms) => new Promise((resolve) => setTimeout(resolve, ms));

  function headers(extra = {}) {
    const result = { 'Content-Type': 'application/json', ...extra };
    if (state.token) result.Authorization = `Bearer ${state.token}`;
    return result;
  }

  async function apiJSON(path, options = {}) {
    const response = await fetch(path, { ...options, headers: headers(options.headers) });
    if (!response.ok) throw new Error(`${path}: HTTP ${response.status}`);
    return response.json();
  }

  function setConnection(ok, message) {
    state.connected = ok;
    const badge = $('#connection');
    badge.textContent = ok ? 'Online' : 'Offline';
    badge.className = `badge ${ok ? 'ok' : 'bad'}`;
    const error = $('#error');
    error.hidden = ok;
    error.textContent = message || '';
  }

  function renderStatus(status) {
    const values = [
      `${status.name || '—'} v${status.version || ''}`,
      status.app || '—',
      `${status.mode || '—'} / ${status.ssid || '—'}`,
      status.ip || '—',
      `${status.uptime_s || 0} s`,
      `${status.free_heap || 0} B`,
    ];
    const cells = $('#status').querySelectorAll('dd');
    values.forEach((value, index) => { cells[index].textContent = value; });
  }

  function renderApps(apps) {
    const container = $('#apps');
    container.replaceChildren();
    apps.forEach((app) => {
      const button = document.createElement('button');
      button.textContent = app.name || app.id;
      button.addEventListener('click', async () => {
        try {
          await apiJSON('/api/v1/launch', {
            method: 'POST',
            body: JSON.stringify({ id: app.id }),
          });
          addEvent({ launched: app.id });
        } catch (error) { addEvent({ error: error.message }); }
      });
      container.appendChild(button);
    });
  }

  function drawDisplay(payload) {
    if (!payload || !payload.data) return;
    const bytes = Uint8Array.from(atob(payload.data), (char) => char.charCodeAt(0));
    const canvas = $('#screen');
    const context = canvas.getContext('2d');
    const width = payload.width || 128;
    const height = payload.height || 64;
    canvas.width = width * 2;
    canvas.height = height * 2;
    context.fillStyle = '#05080f';
    context.fillRect(0, 0, canvas.width, canvas.height);
    context.fillStyle = '#4ade80';
    for (let y = 0; y < height; y++) {
      for (let x = 0; x < width; x++) {
        const byte = bytes[(y >> 3) * width + x];
        if (byte & (1 << (y & 7))) context.fillRect(x * 2, y * 2, 2, 2);
      }
    }
  }

  function addEvent(value) {
    const container = $('#events');
    const row = document.createElement('div');
    if (value && value.error) row.className = 'err';
    const time = document.createElement('span');
    time.className = 'time';
    time.textContent = new Date().toLocaleTimeString();
    row.appendChild(time);
    row.appendChild(document.createTextNode(typeof value === 'string' ? value : JSON.stringify(value)));
    container.prepend(row);
    while (container.childElementCount > 60) container.lastElementChild.remove();
  }

  async function refresh() {
    try {
      const [status, apps, display] = await Promise.all([
        apiJSON('/api/v1/status'),
        apiJSON('/api/v1/apps'),
        apiJSON('/api/v1/display'),
      ]);
      renderStatus(status);
      renderApps(apps.apps || []);
      drawDisplay(display);
      setConnection(true);
    } catch (error) {
      setConnection(false, error.message);
    }
  }

  async function sendInput(key, action = 'short') {
    try {
      await apiJSON('/api/v1/input', {
        method: 'POST',
        body: JSON.stringify({ key, action }),
      });
    } catch (error) { addEvent({ error: error.message }); }
  }

  async function streamEvents() {
    while (true) {
      try {
        const response = await fetch('/api/v1/events', { headers: headers() });
        if (!response.ok) throw new Error(`events: HTTP ${response.status}`);
        const reader = response.body.getReader();
        const decoder = new TextDecoder();
        let buffer = '';
        while (true) {
          const { done, value } = await reader.read();
          if (done) break;
          buffer += decoder.decode(value, { stream: true });
          const blocks = buffer.split('\n\n');
          buffer = blocks.pop() || '';
          for (const block of blocks) {
            const line = block.split('\n').find((item) => item.startsWith('data: '));
            if (!line || line.startsWith('data: :')) continue;
            try { addEvent(JSON.parse(line.slice(6))); } catch (_) { addEvent(line.slice(6)); }
          }
        }
      } catch (error) { addEvent({ stream: error.message }); }
      await sleep(2000);
    }
  }

  $('#save-token').addEventListener('click', () => {
    state.token = $('#token').value.trim();
    if (state.token) localStorage.setItem('flite.token', state.token);
    else localStorage.removeItem('flite.token');
    addEvent({ saved: state.token ? 'API token (local only)' : 'token cleared' });
    refresh();
  });
  $('#token').value = state.token;
  document.querySelectorAll('[data-key]').forEach((button) => {
    button.addEventListener('click', () => sendInput(button.dataset.key, button.dataset.action || 'short'));
  });

  refresh();
  setInterval(refresh, 3000);
  streamEvents();
})();
