import { useMemo } from 'preact/hooks'
import { useStore } from '../ui/store'

export function App() {
  const code = useStore((s) => s.code)
  const setCode = useStore((s) => s.setCode)
  const run = useStore((s) => s.run)
  const running = useStore((s) => s.running)
  const last = useStore((s) => s.last)

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
          <textarea
            style="width:100%;height:300px"
            value={code}
            onInput={(e) => setCode((e.target as HTMLTextAreaElement).value)}
          />
        </div>
        <div class="col">
          <div class={'output' + (last && !last.ok ? ' error' : '')}>{output}</div>
        </div>
      </div>
    </div>
  )
}

