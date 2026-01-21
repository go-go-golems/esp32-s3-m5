import { useEffect, useMemo, useRef } from 'preact/hooks'
import { useStore } from '../ui/store'
import { CodeEditor } from './code_editor'

export function App() {
  const setCode = useStore((s) => s.setCode)
  const run = useStore((s) => s.run)
  const running = useStore((s) => s.running)
  const last = useStore((s) => s.last)
  const wsConnected = useStore((s) => s.wsConnected)
  const encoder = useStore((s) => s.encoder)
  const lastClick = useStore((s) => s.lastClick)
  const connectWs = useStore((s) => s.connectWs)
  const wsHistory = useStore((s) => s.wsHistory)
  const clearWsHistory = useStore((s) => s.clearWsHistory)
  const initialCode = useMemo(() => useStore.getState().code, [])
  const wsLogRef = useRef<HTMLDivElement>(null)

  useEffect(() => {
    connectWs()
  }, [connectWs])

  useEffect(() => {
    const el = wsLogRef.current
    if (!el) return
    const nearBottom = el.scrollTop + el.clientHeight >= el.scrollHeight - 24
    if (nearBottom) el.scrollTop = el.scrollHeight
  }, [wsHistory.length])

  const output = useMemo(() => {
    if (!last) return ''
    if (last.ok) return last.output ?? ''
    return last.error ?? ''
  }, [last])

  const jsHelp = useMemo(
    () => `What JS can do (0048):

- Run arbitrary JS snippets via "Run" (POST /api/js/eval)
- JS VM is stateful across runs (globals persist until reboot/reset)
- Ctrl/Cmd-Enter runs from the editor

Builtins / notes:
- Return value of the last expression is shown in the Output pane
- Exceptions are shown in the Output pane
- print(...) exists, but currently prints to the device console (USB Serial/JTAG), not to the browser output
- load("/spiffs/foo.js") exists if SPIFFS is present (see partitions.csv); file size limit ~32 KiB
- Date.now() and performance.now() exist (ms since boot)
- gc() triggers a JS GC cycle

Phase 2B (callbacks):
- encoder.on('delta', (ev)=>{...})  where ev={pos,delta,ts_ms,seq}
- encoder.on('click', (ev)=>{...})  where ev={kind,ts_ms} (kind:0 single,1 double,2 long)
- encoder.off('delta'|'click')
- Callbacks run on-device (best-effort) and are time-bounded; exceptions are logged to the device console

Phase 2C (events to browser):
- emit(topic, payload) records a structured event in the JS VM
- After eval/callback, the VM flushes and the device broadcasts a WS frame:
  {type:"js_events", source:"eval"|"callback", events:[{topic,payload,ts_ms}], dropped?:N}

Device console:
- There is also: 0048> js eval <code...>   (prints JSON result)

WebSocket:
- /ws pushes device->browser JSON frames (encoder snapshots + click events today)
- The UI below shows a bounded history of raw WS frames for debugging
`,
    []
  )

  return (
    <div>
      <div class="toolbar">
        <button onClick={() => run()} disabled={running}>
          {running ? 'Running…' : 'Run'}
        </button>
        <a href="/api/status" target="_blank" rel="noreferrer">
          /api/status
        </a>
        <button onClick={() => clearWsHistory()} title="Clear WS event history">
          Clear WS
        </button>
        <span class={'ws ' + (wsConnected ? 'ok' : 'bad')}>{wsConnected ? 'WS: connected' : 'WS: disconnected'}</span>
        {encoder ? (
          <span class="enc">
            enc pos={encoder.pos} delta={encoder.delta}
          </span>
        ) : (
          <span class="enc">enc: -</span>
        )}
        {lastClick ? <span class="enc">click kind={lastClick.kind}</span> : null}
      </div>
      <details class="help" open>
        <summary>JS help</summary>
        <pre>{jsHelp}</pre>
      </details>
      <div class="row">
        <div class="col">
          <CodeEditor initialValue={initialCode} onChange={setCode} onRun={run} />
          <div class="hint">Tip: Ctrl/Cmd-Enter runs.</div>
        </div>
        <div class="col">
          <div class={'output' + (last && !last.ok ? ' error' : '')}>{output}</div>
        </div>
      </div>
      <div class="events">
        <div class="eventsHead">
          <div class="eventsTitle">WebSocket event history (last {wsHistory.length})</div>
          <div class="eventsHint">bounded to 200 frames</div>
        </div>
        <div class="eventLog" ref={wsLogRef}>
          {wsHistory.map((e) => {
            const t = new Date(e.rx_ms).toISOString().slice(11, 19)
            const typ = e.parsed && typeof e.parsed === 'object' ? String((e.parsed as any).type ?? 'json') : 'raw'
            return (
              <div class="eventLine" key={e.id}>
                <span class="eventTime">{t}</span>
                <span class="eventType">{typ}</span>
                <span class={'eventBody' + (e.parse_error ? ' bad' : '')}>{e.raw || e.parse_error || ''}</span>
              </div>
            )
          })}
        </div>
      </div>
    </div>
  )
}
