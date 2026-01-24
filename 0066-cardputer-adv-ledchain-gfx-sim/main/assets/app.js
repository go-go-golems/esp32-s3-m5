function $(id) {
  return document.getElementById(id);
}

function setConn(ok, msg) {
  const pill = $('ws_status');
  if (!pill) return;
  pill.classList.remove('ok', 'bad');
  pill.classList.add(ok ? 'ok' : 'bad');
  pill.textContent = ok ? `ws: ${msg}` : `ws: ${msg}`;
}

function safeJsonParse(s) {
  try {
    return JSON.parse(s);
  } catch {
    return null;
  }
}

async function getStatus() {
  const res = await fetch('/api/status', { cache: 'no-store' });
  const j = await res.json();
  $('status').textContent = JSON.stringify(j, null, 2);
  if (j && j.pattern && j.pattern.global_brightness_pct) $('bri').value = j.pattern.global_brightness_pct;
  if (j && j.frame_ms) $('frame_ms').value = j.frame_ms;
  if (j && j.pattern && j.pattern.type) $('type').value = j.pattern.type;
}

async function apply() {
  const type = $('type').value;
  const bri = Number($('bri').value);
  const frame_ms = Number($('frame_ms').value);

  const body = { type, brightness_pct: bri, frame_ms };
  const res = await fetch('/api/apply', {
    method: 'POST',
    headers: { 'content-type': 'application/json' },
    body: JSON.stringify(body),
  });
  const j = await res.json().catch(() => ({ ok: false }));
  $('status').textContent = JSON.stringify(j, null, 2);
}

async function jsEval() {
  const code = $('js_code').value || '';
  $('js_result').textContent = 'running...';

  const res = await fetch('/api/js/eval', {
    method: 'POST',
    headers: { 'content-type': 'text/plain; charset=utf-8' },
    body: code,
  });

  const text = await res.text();
  const j = safeJsonParse(text);
  if (!j) {
    $('js_result').textContent = text;
    return;
  }

  const out = [];
  out.push(`ok=${j.ok} timed_out=${j.timed_out}`);
  if (j.error) out.push(`error:\n${j.error}`);
  if (j.output) out.push(`output:\n${j.output}`);
  $('js_result').textContent = out.join('\n\n');
}

function appendEvent(line) {
  const box = $('ws_events');
  if (!box) return;
  const cur = box.textContent || '';
  const next = (cur.length > 12000) ? cur.slice(cur.length - 12000) : cur;
  box.textContent = (next ? (next + '\n') : '') + line;
  box.scrollTop = box.scrollHeight;
}

function connectWs() {
  const proto = (location.protocol === 'https:') ? 'wss' : 'ws';
  const url = `${proto}://${location.host}/ws`;
  let ws = null;

  function start() {
    try {
      ws = new WebSocket(url);
    } catch (e) {
      setConn(false, 'failed');
      return;
    }

    ws.onopen = () => setConn(true, 'connected');
    ws.onclose = () => {
      setConn(false, 'disconnected');
      setTimeout(start, 750);
    };
    ws.onerror = () => setConn(false, 'error');
    ws.onmessage = (ev) => {
      const s = String(ev.data || '');
      const j = safeJsonParse(s);
      if (j) {
        appendEvent(JSON.stringify(j));
      } else {
        appendEvent(s);
      }
    };
  }

  start();
}

function installDefaults() {
  const demo =
`// 0066 Web JS IDE demo (ES5)
// Try: emit('log', { msg: 'hi' })

emit('log', { msg: 'hello from JS', ts: Date.now() });
sim.setBrightness(25);
sim.setPattern('rainbow');
`;
  $('js_code').value = demo;
}

function main() {
  $('refresh').addEventListener('click', getStatus);
  $('apply').addEventListener('click', apply);
  $('js_run').addEventListener('click', jsEval);
  $('js_clear').addEventListener('click', () => { $('js_result').textContent = ''; });
  $('ws_clear').addEventListener('click', () => { $('ws_events').textContent = ''; });

  installDefaults();
  connectWs();
  getStatus().catch(() => {});
}

window.addEventListener('load', main);

