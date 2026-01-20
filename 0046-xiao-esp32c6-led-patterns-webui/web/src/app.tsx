import { useEffect, useMemo, useState } from 'preact/hooks'
import { useLedStore } from './store/led'

export function App() {
  const status = useLedStore((s) => s.status)
  const err = useLedStore((s) => s.err)
  const refresh = useLedStore((s) => s.refresh)
  const setPattern = useLedStore((s) => s.setPattern)
  const setBrightness = useLedStore((s) => s.setBrightness)
  const setFrame = useLedStore((s) => s.setFrame)
  const pause = useLedStore((s) => s.pause)
  const resume = useLedStore((s) => s.resume)
  const clear = useLedStore((s) => s.clear)

  const [pendingBrightness, setPendingBrightness] = useState<number>(25)
  const [pendingFrame, setPendingFrame] = useState<number>(16)

  const pattern = status?.pattern.type ?? 'rainbow'
  const bri = status?.pattern.global_brightness_pct ?? pendingBrightness
  const frame = status?.frame_ms ?? pendingFrame

  useEffect(() => {
    setPendingBrightness(bri)
    setPendingFrame(frame)
  }, [bri, frame])

  const patterns = useMemo(() => ['off', 'rainbow', 'chase', 'breathing', 'sparkle'], [])

  useEffect(() => {
    refresh()
    const t = setInterval(() => refresh(), 500)
    return () => clearInterval(t)
  }, [])

  return (
    <div class="wrap">
      <h1>ESP32-C6 LED Patterns</h1>

      <div class="card">
        <h2>Status</h2>
        {err ? <pre class="err">{err}</pre> : null}
        {status ? (
          <pre>{JSON.stringify(status, null, 2)}</pre>
        ) : (
          <div>loading… (connect Wi‑Fi, then browse to device IP)</div>
        )}
      </div>

      <div class="card">
        <h2>Controls</h2>

        <div class="row">
          <label>
            Pattern{' '}
            <select
              value={pattern}
              onChange={(e) => setPattern((e.currentTarget as HTMLSelectElement).value)}
            >
              {patterns.map((p) => (
                <option value={p} key={p}>
                  {p}
                </option>
              ))}
            </select>
          </label>
        </div>

        <div class="row">
          <label>
            Brightness {pendingBrightness}%
            <input
              type="range"
              min={1}
              max={100}
              value={pendingBrightness}
              onInput={(e) =>
                setPendingBrightness(parseInt((e.currentTarget as HTMLInputElement).value, 10))
              }
              onChange={() => setBrightness(pendingBrightness)}
            />
          </label>
        </div>

        <div class="row">
          <label>
            Frame (ms){' '}
            <input
              type="number"
              min={1}
              max={1000}
              value={pendingFrame}
              onInput={(e) => setPendingFrame(parseInt((e.currentTarget as HTMLInputElement).value, 10))}
            />
          </label>
          <button onClick={() => setFrame(pendingFrame)}>apply</button>
        </div>

        <div class="row">
          <button onClick={() => pause()}>pause</button>
          <button onClick={() => resume()}>resume</button>
          <button onClick={() => clear()}>clear</button>
        </div>
      </div>
    </div>
  )
}
