import { useEffect, useMemo, useState } from 'preact/hooks'
import './app.css'
import { apiGraphicsList, apiGraphicsUpload, apiStatus, type GraphicsEntry, type Status } from './api'
import { useWsStore } from './store/ws'

function sanitizeName(name: string): string {
  // Firmware-side validation rejects slashes and "..". Keep it simple and stable.
  return name.replace(/[^a-zA-Z0-9._-]/g, '_')
}

export function App() {
  const ws = useWsStore()
  const [status, setStatus] = useState<Status | null>(null)
  const [graphics, setGraphics] = useState<GraphicsEntry[]>([])
  const [uploadFile, setUploadFile] = useState<File | null>(null)
  const [uploadError, setUploadError] = useState<string | null>(null)
  const [uploading, setUploading] = useState(false)

  const [txText, setTxText] = useState('')
  const enc = useMemo(() => new TextEncoder(), [])

  async function refresh() {
    const [s, g] = await Promise.all([apiStatus(), apiGraphicsList()])
    setStatus(s)
    setGraphics(g)
  }

  useEffect(() => {
    ws.connect()
    refresh().catch(() => {})
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [])

  async function doUpload() {
    if (!uploadFile) return
    setUploading(true)
    setUploadError(null)
    try {
      const name = sanitizeName(uploadFile.name || 'upload.png')
      await apiGraphicsUpload(name, uploadFile)
      await refresh()
    } catch (e) {
      setUploadError(e instanceof Error ? e.message : String(e))
    } finally {
      setUploading(false)
    }
  }

  function sendLine() {
    if (!txText) return
    const bytes = enc.encode(txText + '\r')
    ws.sendBytes(bytes)
    setTxText('')
  }

  return (
    <div class="page">
      <header class="header">
        <div>
          <div class="title">AtomS3R Web UI</div>
          <div class="subtitle">Graphics upload + WebSocket terminal</div>
        </div>
        <div class="status">
          <span>WS: {ws.connected ? 'connected' : 'disconnected'}</span>
          <button class="btn" onClick={() => (ws.connected ? ws.disconnect() : ws.connect())}>
            {ws.connected ? 'Disconnect' : 'Connect'}
          </button>
          <button class="btn" onClick={() => refresh()}>
            Refresh
          </button>
        </div>
      </header>

      <section class="card">
        <div class="cardTitle">Device status</div>
        <pre class="mono">
          {status ? JSON.stringify(status, null, 2) : 'loading...'}
        </pre>
      </section>

      <section class="grid">
        <div class="card">
          <div class="cardTitle">Upload PNG</div>
          <div class="row">
            <input
              type="file"
              accept="image/png"
              onChange={(e) => setUploadFile((e.currentTarget.files && e.currentTarget.files[0]) || null)}
            />
            <button class="btn" disabled={!uploadFile || uploading} onClick={() => void doUpload()}>
              {uploading ? 'Uploading…' : 'Upload'}
            </button>
          </div>
          {uploadError ? <div class="error mono">{uploadError}</div> : null}
          <div class="hint">Uploaded image is auto-rendered on the device display.</div>
        </div>

        <div class="card">
          <div class="cardTitle">Graphics</div>
          <ul class="list mono">
            {graphics.map((g) => (
              <li key={g.name}>{g.name}</li>
            ))}
          </ul>
        </div>
      </section>

      <section class="card">
        <div class="cardTitle">Terminal (UART)</div>
        <div class="row">
          <input
            class="mono input"
            value={txText}
            onInput={(e) => setTxText(e.currentTarget.value)}
            onKeyDown={(e) => {
              if (e.key === 'Enter') sendLine()
            }}
            placeholder="Type bytes (ASCII) and press Enter…"
          />
          <button class="btn" onClick={() => sendLine()}>
            Send
          </button>
          <button class="btn" onClick={() => ws.clearTerminal()}>
            Clear
          </button>
        </div>
        <pre class="terminal mono">{ws.terminalText || '(no data yet)'}</pre>
      </section>

      <section class="card">
        <div class="cardTitle">Button events</div>
        <ul class="list mono">
          {ws.buttonEvents.slice(-10).map((e, idx) => (
            <li key={`${e.ts_ms}-${idx}`}>button @ {e.ts_ms}ms</li>
          ))}
        </ul>
      </section>
    </div>
  )
}
