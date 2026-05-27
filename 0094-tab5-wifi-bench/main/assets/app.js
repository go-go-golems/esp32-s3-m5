/* Tab5 WiFi Benchmark - Browser benchmark runner */

const allResults = [];
let runCounter = 0;

function $(id) { return document.getElementById(id); }

function setStatus(msg) { $('status').textContent = msg; }

function formatBytes(b) {
    if (b >= 1048576) return (b / 1048576).toFixed(1) + ' MB';
    if (b >= 1024) return (b / 1024).toFixed(1) + ' KB';
    return b + ' B';
}

function getBaseUrl() { return $('baseUrl').value; }
function getSize() { return parseInt($('sizeSelect').value); }
function getCompress() { return $('compressSelect').value; }

function disableButtons(v) {
    $('btnUpload').disabled = v;
    $('btnDownload').disabled = v;
    $('btnPing').disabled = v;
    $('btnAuto').disabled = v;
}

/* Generate a payload with an incrementing byte pattern.
 * Compressible but not trivially so (similar to real UI images). */
function generatePayload(size) {
    const buf = new Uint8Array(size);
    for (let i = 0; i < size; i++) buf[i] = i & 0xFF;
    return buf;
}

async function compressDeflate(data) {
    const cs = new CompressionStream('deflate');
    const writer = cs.writable.getWriter();
    const reader = cs.readable.getReader();
    writer.write(data);
    writer.close();
    const chunks = [];
    while (true) {
        const r = await reader.read();
        if (r.done) break;
        chunks.push(r.value);
    }
    let totalLen = 0;
    for (const c of chunks) totalLen += c.length;
    const out = new Uint8Array(totalLen);
    let off = 0;
    for (const c of chunks) { out.set(c, off); off += c.length; }
    return out;
}

/* ---- Upload benchmark ---- */
async function runUpload() {
    const size = getSize();
    const compress = getCompress();
    const base = getBaseUrl();
    const payload = generatePayload(size);

    disableButtons(true);
    setStatus('Generating ' + formatBytes(size) + ' ' + compress + '...');

    let body, headers = {};
    if (compress === 'deflate') {
        setStatus('Compressing ' + formatBytes(size) + '...');
        body = await compressDeflate(payload);
        headers['Content-Encoding'] = 'deflate';
    } else {
        body = payload;
    }

    const url = base + '/api/bench/upload?size=' + size;
    setStatus('Uploading ' + formatBytes(body.length) + '...');

    const t0 = performance.now();
    const resp = await fetch(url, { method: 'POST', body, headers });
    const t1 = performance.now();
    const data = await resp.json();

    runCounter++;
    const row = {
        id: runCounter,
        mode: base.includes('4.1') ? 'SoftAP' : 'STA',
        direction: 'upload',
        size: size,
        compress: compress,
        browser_ms: Math.round(t1 - t0),
        recv_us: data.timing_us ? data.timing_us.recv : null,
        decomp_us: data.timing_us ? data.timing_us.decompress : null,
        total_us: data.timing_us ? data.timing_us.total : null,
        recv_kbps: data.recv_throughput_kbps || 0,
        segments: data.segments || [],
        rssi: data.system ? data.system.rssi : null,
        payload_bytes: data.payload_bytes,
        decompressed_bytes: data.decompressed_bytes,
    };
    allResults.push(row);
    addResultRow(row);
    drawSegmentChart(row.segments);
    setStatus('Upload done: ' + row.browser_ms + ' ms browser, ' +
              (row.recv_us ? Math.round(row.recv_us / 1000) + ' ms server' : 'N/A') +
              ', ' + Math.round(row.recv_kbps) + ' kbps');
    disableButtons(false);
    return row;
}

/* ---- Download benchmark ---- */
async function runDownload() {
    const size = getSize();
    const base = getBaseUrl();

    disableButtons(true);
    setStatus('Downloading ' + formatBytes(size) + '...');

    const url = base + '/api/bench/download?size=' + size;
    const t0 = performance.now();
    const resp = await fetch(url);
    const buf = await resp.arrayBuffer();
    const t1 = performance.now();

    const elapsed = t1 - t0;
    const kbps = (buf.byteLength * 8) / (elapsed / 1000) / 1000;

    runCounter++;
    const row = {
        id: runCounter,
        mode: base.includes('4.1') ? 'SoftAP' : 'STA',
        direction: 'download',
        size: size,
        compress: 'raw',
        browser_ms: Math.round(elapsed),
        recv_us: null,
        decomp_us: null,
        total_us: null,
        recv_kbps: kbps,
        segments: [],
        rssi: null,
        payload_bytes: buf.byteLength,
        decompressed_bytes: buf.byteLength,
    };
    allResults.push(row);
    addResultRow(row);
    setStatus('Download done: ' + row.browser_ms + ' ms, ' + Math.round(kbps) + ' kbps');
    disableButtons(false);
    return row;
}

/* ---- Ping benchmark ---- */
async function runPing() {
    const size = getSize() > 1024 ? 1024 : getSize();
    const base = getBaseUrl();
    const payload = generatePayload(size);

    disableButtons(true);
    setStatus('Pinging with ' + formatBytes(size) + '...');

    const url = base + '/api/bench/ping';
    const t0 = performance.now();
    const resp = await fetch(url, { method: 'POST', body: payload });
    const buf = await resp.arrayBuffer();
    const t1 = performance.now();

    const rtt = t1 - t0;
    runCounter++;
    const row = {
        id: runCounter,
        mode: base.includes('4.1') ? 'SoftAP' : 'STA',
        direction: 'ping',
        size: size,
        compress: 'raw',
        browser_ms: Math.round(rtt),
        recv_us: null, decomp_us: null, total_us: null,
        recv_kbps: (size * 8) / (rtt / 1000) / 1000,
        segments: [], rssi: null,
        payload_bytes: size, decompressed_bytes: size,
    };
    allResults.push(row);
    addResultRow(row);
    setStatus('Ping RTT: ' + rtt.toFixed(1) + ' ms (' + buf.byteLength + ' bytes echoed)');
    disableButtons(false);
    return row;
}

/* ---- Auto suite ---- */
async function runAutoSuite() {
    const sizes = [1024, 10240, 102400, 512000, 1048576, 1843200];
    const compressions = ['raw', 'deflate'];
    const base = getBaseUrl();

    disableButtons(true);
    let count = 0;
    const total = sizes.length * compressions.length;

    for (const compress of compressions) {
        for (const size of sizes) {
            count++;
            setStatus('Auto suite ' + count + '/' + total + ': ' +
                       formatBytes(size) + ' ' + compress);

            /* Skip deflate for sizes < 10 KB (overhead dominates) */
            if (compress === 'deflate' && size < 10240) continue;

            $('sizeSelect').value = size;
            $('compressSelect').value = compress;
            await runUpload();
            /* Small delay between runs to let TCP settle. */
            await new Promise(r => setTimeout(r, 500));
        }
    }

    setStatus('Auto suite complete: ' + allResults.length + ' results');
    disableButtons(false);
}

/* ---- UI helpers ---- */

function addResultRow(row) {
    const tbody = $('resultsBody');
    const tr = document.createElement('tr');
    tr.innerHTML =
        '<td>' + row.id + '</td>' +
        '<td class="str">' + row.mode + '</td>' +
        '<td class="str">' + row.direction + '</td>' +
        '<td>' + formatBytes(row.size) + '</td>' +
        '<td class="str">' + row.compress + '</td>' +
        '<td>' + row.browser_ms + '</td>' +
        '<td>' + (row.recv_us != null ? Math.round(row.recv_us / 1000) : '-') + '</td>' +
        '<td>' + (row.decomp_us != null ? Math.round(row.decomp_us / 1000) : '-') + '</td>' +
        '<td>' + (row.total_us != null ? Math.round(row.total_us / 1000) : '-') + '</td>' +
        '<td>' + Math.round(row.recv_kbps) + '</td>' +
        '<td>' + row.segments.length + '</td>' +
        '<td>' + (row.rssi != null ? row.rssi : '-') + '</td>';
    tbody.appendChild(tr);
}

function drawSegmentChart(segments) {
    if (!segments || segments.length === 0) {
        $('segments').textContent = 'No segment data for this run.';
        return;
    }

    const canvas = $('chart');
    const ctx = canvas.getContext('2d');
    canvas.width = canvas.clientWidth * (window.devicePixelRatio || 1);
    canvas.height = canvas.clientHeight * (window.devicePixelRatio || 1);
    ctx.scale(window.devicePixelRatio || 1, window.devicePixelRatio || 1);
    const W = canvas.clientWidth;
    const H = canvas.clientHeight;

    ctx.fillStyle = '#111';
    ctx.fillRect(0, 0, W, H);

    const t0 = segments[0].t;
    const tLast = segments[segments.length - 1].t;
    const dtTotal = tLast - t0 || 1;
    const maxBytes = Math.max(...segments.map(s => s.b));

    /* Draw bars for each segment: width = proportional to time, height = proportional to bytes */
    ctx.fillStyle = '#0af';
    for (let i = 0; i < segments.length; i++) {
        const x = ((segments[i].t - t0) / dtTotal) * W;
        const barH = (segments[i].b / maxBytes) * H * 0.9;
        ctx.fillRect(x - 1, H - barH, 3, barH);
    }

    /* Summary text */
    $('segments').textContent =
        segments.length + ' segments, ' +
        Math.round(dtTotal / 1000) + ' ms total, ' +
        'avg ' + Math.round(segments.reduce((a, s) => a + s.b, 0) / segments.length) + ' bytes/seg';
}

function exportResults() {
    const json = JSON.stringify(allResults, null, 2);
    const blob = new Blob([json], { type: 'application/json' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = 'tab5-bench-results.json';
    a.click();
    URL.revokeObjectURL(url);
    setStatus('Exported ' + allResults.length + ' results');
}
