/* Tab5 UI Screen Viewer - browser-side image conversion and upload */

(function () {
  'use strict';

  const TARGET_W = 1280;
  const TARGET_H = 720;

  const dropzone = document.getElementById('dropzone');
  const fileInput = document.getElementById('file-input');
  const progressBar = document.getElementById('progress-bar');
  const progressInner = document.getElementById('progress-bar-inner');
  const statusEl = document.getElementById('status');
  const btnClear = document.getElementById('btn-clear');
  const screenInfo = document.getElementById('screen-info');
  const canvas = document.getElementById('canvas');
  const ctx = canvas.getContext('2d', { willReadFrequently: true });

  /* ---- Helpers ---- */

  function setStatus(msg) { statusEl.textContent = msg; }
  function showProgress(pct) {
    progressBar.style.display = 'block';
    progressInner.style.width = pct + '%';
  }
  function hideProgress() {
    progressBar.style.display = 'none';
    progressInner.style.width = '0%';
  }

  /* ---- Fetch screen info ---- */

  async function fetchScreenInfo() {
    try {
      const r = await fetch('/api/screen');
      const d = await r.json();
      if (d.ok) {
        screenInfo.textContent = `Screen: ${d.width}x${d.height} ${d.format} (${(d.buf_size / 1024 / 1024).toFixed(2)} MB)`;
      }
    } catch (_) { /* ignore */ }
  }
  fetchScreenInfo();

  /* ---- RGBA -> RGB565 conversion ---- */

  function rgbaToRgb565(rgbaData, w, h) {
    const numPixels = w * h;
    const out = new Uint8Array(numPixels * 2);
    for (let i = 0; i < numPixels; i++) {
      const r = rgbaData[i * 4];
      const g = rgbaData[i * 4 + 1];
      const b = rgbaData[i * 4 + 2];
      // RGB565: R=5bit, G=6bit, B=5bit, little-endian
      const r5 = (r >> 3) & 0x1F;
      const g6 = (g >> 2) & 0x3F;
      const b5 = (b >> 3) & 0x1F;
      const rgb565 = (r5 << 11) | (g6 << 5) | b5;
      // Little-endian
      out[i * 2] = rgb565 & 0xFF;
      out[i * 2 + 1] = (rgb565 >> 8) & 0xFF;
    }
    return out;
  }

  /* ---- Image processing pipeline ---- */

  function processImage(file) {
    return new Promise((resolve, reject) => {
      const img = new Image();
      img.onload = function () {
        URL.revokeObjectURL(img.src);

        // Resize to target dimensions on canvas
        canvas.width = TARGET_W;
        canvas.height = TARGET_H;
        ctx.fillStyle = '#000000';
        ctx.fillRect(0, 0, TARGET_W, TARGET_H);
        ctx.drawImage(img, 0, 0, TARGET_W, TARGET_H);

        // Read pixel data
        const imageData = ctx.getImageData(0, 0, TARGET_W, TARGET_H);
        const rgb565 = rgbaToRgb565(imageData.data, TARGET_W, TARGET_H);
        resolve(rgb565);
      };
      img.onerror = function () {
        URL.revokeObjectURL(img.src);
        reject(new Error('Failed to load image'));
      };
      img.src = URL.createObjectURL(file);
    });
  }

  /* ---- Upload RGB565 data (gzip compressed) ---- */

  async function uploadRgb565(data) {
    showProgress(10);
    setStatus('Compressing...');

    /* Compress the RGB565 data using the browser's built-in CompressionStream.
     * We use 'deflate' (zlib format, RFC 1950) because the ESP32 ROM miniz
     * library provides mz_uncompress() for zlib format directly — no manual
     * header parsing needed.  For restricted-palette UIs this gives 10-200x
     * compression, turning a 1.8 MB upload into 10-200 KB. */
    let compressed;
    try {
      const cs = new CompressionStream('deflate');
      const writer = cs.writable.getWriter();
      const reader = cs.readable.getReader();
      writer.write(data);
      writer.close();
      const chunks = [];
      while (true) {
        const { done, value } = await reader.read();
        if (done) break;
        chunks.push(value);
      }
      const totalLen = chunks.reduce((s, c) => s + c.byteLength, 0);
      compressed = new Uint8Array(totalLen);
      let offset = 0;
      for (const c of chunks) {
        compressed.set(c, offset);
        offset += c.byteLength;
      }
    } catch (_) {
      /* CompressionStream not available (rare); fall back to raw. */
      compressed = new Uint8Array(data.buffer);
    }

    const isGz = (compressed !== new Uint8Array(data.buffer));
    showProgress(30);
    setStatus(`Uploading ${(compressed.byteLength / 1024).toFixed(0)} KB` +
              (isGz ? ` (gzip, ${((1 - compressed.byteLength / data.byteLength) * 100).toFixed(0)}% smaller)` : ''));

    try {
      const headers = { 'Content-Type': 'application/octet-stream' };
      if (isGz) headers['Content-Encoding'] = 'deflate';
      const r = await fetch('/api/upload', {
        method: 'POST',
        body: compressed,
        headers,
      });
      showProgress(100);
      const d = await r.json();
      if (d.ok) {
        setStatus('[OK] Image uploaded - check Tab5 display');
      } else {
        setStatus('[ERR] Upload failed: ' + (d.error || 'unknown'));
      }
    } catch (e) {
      setStatus('[ERR] Network error: ' + e.message);
    }
    setTimeout(hideProgress, 2000);
  }

  /* ---- Clear screen ---- */

  async function clearScreen() {
    try {
      const r = await fetch('/api/clear', { method: 'POST' });
      const d = await r.json();
      if (d.ok) {
        setStatus('[CLR] Screen cleared');
      }
    } catch (_) {
      setStatus('[ERR] Clear failed');
    }
  }

  /* ---- Event handlers ---- */

  async function handleFile(file) {
    if (!file || !file.type.startsWith('image/')) {
      setStatus('[WARN] Please select an image file');
      return;
    }
    setStatus('Processing: ' + file.name + '...');
    showProgress(2);
    try {
      const rgb565 = await processImage(file);
      showProgress(5);
      setStatus(`Converted: ${TARGET_W}x${TARGET_H} RGB565 (${(rgb565.byteLength / 1024).toFixed(0)} KB)`);
      await uploadRgb565(rgb565);
    } catch (e) {
      setStatus('[ERR] ' + e.message);
      hideProgress();
    }
  }

  // Drag-and-drop
  dropzone.addEventListener('dragover', function (e) {
    e.preventDefault();
    dropzone.classList.add('dragover');
  });
  dropzone.addEventListener('dragleave', function () {
    dropzone.classList.remove('dragover');
  });
  dropzone.addEventListener('drop', function (e) {
    e.preventDefault();
    dropzone.classList.remove('dragover');
    if (e.dataTransfer.files.length > 0) {
      handleFile(e.dataTransfer.files[0]);
    }
  });

  // Click to browse
  dropzone.addEventListener('click', function () {
    fileInput.click();
  });
  fileInput.addEventListener('change', function () {
    if (fileInput.files.length > 0) {
      handleFile(fileInput.files[0]);
    }
    fileInput.value = '';
  });

  // Clear button
  btnClear.addEventListener('click', clearScreen);

})();
