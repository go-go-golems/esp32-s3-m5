const form = document.getElementById('echoForm');
const input = document.getElementById('echoInput');
const submitBtn = document.getElementById('submitBtn');
const clearBtn = document.getElementById('clearBtn');
const errorEl = document.getElementById('error');
const statusEl = document.getElementById('status');
const echoOutput = document.getElementById('echoOutput');
const stateOutput = document.getElementById('stateOutput');

function setError(message) {
  errorEl.textContent = message || '';
}

function renderState(state) {
  const text = state && typeof state.text === 'string' ? state.text : '';
  const version = state && typeof state.version === 'number' ? state.version : 0;
  input.value = text;
  echoOutput.textContent = text.length ? text : '(empty)';
  stateOutput.textContent = JSON.stringify({ ok: true, version, text }, null, 2);
  statusEl.textContent = `Version ${version} • ${text.length} bytes`;
}

async function fetchState() {
  const response = await fetch('/api/state', { cache: 'no-store' });
  if (!response.ok) {
    throw new Error(`GET /api/state failed (${response.status})`);
  }
  return await response.json();
}

async function postText(text) {
  const response = await fetch('/api/text', {
    method: 'POST',
    headers: {
      'Content-Type': 'text/plain; charset=utf-8',
    },
    body: text,
  });
  if (!response.ok) {
    let message = `POST /api/text failed (${response.status})`;
    try {
      const data = await response.json();
      if (data && data.error) {
        message = data.error;
      }
    } catch (_) {
      /* ignore */
    }
    throw new Error(message);
  }
  return await response.json();
}

async function refresh() {
  const state = await fetchState();
  renderState(state);
}

form.addEventListener('submit', async (event) => {
  event.preventDefault();
  setError('');
  submitBtn.disabled = true;
  clearBtn.disabled = true;
  try {
    const state = await postText(input.value);
    renderState(state);
  } catch (error) {
    setError(error instanceof Error ? error.message : String(error));
  } finally {
    submitBtn.disabled = false;
    clearBtn.disabled = false;
  }
});

clearBtn.addEventListener('click', async () => {
  input.value = '';
  submitBtn.disabled = true;
  clearBtn.disabled = true;
  setError('');
  try {
    const state = await postText('');
    renderState(state);
  } catch (error) {
    setError(error instanceof Error ? error.message : String(error));
  } finally {
    submitBtn.disabled = false;
    clearBtn.disabled = false;
  }
});

refresh().catch((error) => {
  setError(error instanceof Error ? error.message : String(error));
  statusEl.textContent = 'Offline';
});
