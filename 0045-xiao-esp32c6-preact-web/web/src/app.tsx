import { useEffect, useState } from 'preact/hooks'
import { useCounterStore } from './store/counter'

type Status = {
  ok: boolean
  chip?: string
  ip?: string
  rssi?: number
  free_heap?: number
}

export function App() {
  const count = useCounterStore((s) => s.count)
  const inc = useCounterStore((s) => s.inc)
  const dec = useCounterStore((s) => s.dec)
  const reset = useCounterStore((s) => s.reset)

  const [status, setStatus] = useState<Status | null>(null)
  const [statusErr, setStatusErr] = useState<string | null>(null)

  useEffect(() => {
    let aborted = false
    fetch('/api/status')
      .then((r) => r.json())
      .then((j) => {
        if (aborted) return
        setStatus(j as Status)
      })
      .catch((e) => {
        if (aborted) return
        setStatusErr(String(e))
      })
    return () => {
      aborted = true
    }
  }, [])

  return (
    <div class="wrap">
      <h1>ESP32-C6 Counter</h1>
      <div class="card">
        <div class="count">{count}</div>
        <div class="row">
          <button onClick={() => dec()}>-</button>
          <button onClick={() => reset()}>reset</button>
          <button onClick={() => inc()}>+</button>
        </div>
      </div>

      <h2>Device status</h2>
      {statusErr ? <pre class="err">{statusErr}</pre> : null}
      {status ? <pre>{JSON.stringify(status, null, 2)}</pre> : <div>loading…</div>}
    </div>
  )
}

