import { useEffect, useMemo } from 'preact/hooks'
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
  const initialCode = useMemo(() => useStore.getState().code, [])

  useEffect(() => {
    connectWs()
  }, [connectWs])

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

Device console:
- There is also: 0048> js eval <code...>   (prints JSON result)
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
    </div>
  )
}
