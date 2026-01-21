import { useMemo } from 'preact/hooks'
import { useStore } from '../ui/store'
import { CodeEditor } from './code_editor'

export function App() {
  const setCode = useStore((s) => s.setCode)
  const run = useStore((s) => s.run)
  const running = useStore((s) => s.running)
  const last = useStore((s) => s.last)
  const initialCode = useMemo(() => useStore.getState().code, [])

  const output = useMemo(() => {
    if (!last) return ''
    if (last.ok) return last.output ?? ''
    return last.error ?? ''
  }, [last])

  return (
    <div>
      <div class="toolbar">
        <button onClick={() => run()} disabled={running}>
          {running ? 'Running…' : 'Run'}
        </button>
        <a href="/api/status" target="_blank" rel="noreferrer">
          /api/status
        </a>
      </div>
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
