import { useEffect, useMemo, useState } from 'preact/hooks'
import { useLedStore } from './store/led'

export function App() {
  const status = useLedStore((s) => s.status)
  const err = useLedStore((s) => s.err)
  const refresh = useLedStore((s) => s.refresh)
  const setPattern = useLedStore((s) => s.setPattern)
  const setBrightness = useLedStore((s) => s.setBrightness)
  const setFrame = useLedStore((s) => s.setFrame)
  const setRainbow = useLedStore((s) => s.setRainbow)
  const setChase = useLedStore((s) => s.setChase)
  const setBreathing = useLedStore((s) => s.setBreathing)
  const setSparkle = useLedStore((s) => s.setSparkle)
  const pause = useLedStore((s) => s.pause)
  const resume = useLedStore((s) => s.resume)
  const clear = useLedStore((s) => s.clear)

  const [pendingBrightness, setPendingBrightness] = useState<number>(25)
  const [pendingFrame, setPendingFrame] = useState<number>(16)

  const [rainbowCfg, setRainbowCfg] = useState({ speed: 5, sat: 100, spread: 10 })
  const [chaseCfg, setChaseCfg] = useState({
    speed: 30,
    tail: 16,
    gap: 8,
    trains: 1,
    fg: '#ffffff',
    bg: '#000000',
    dir: 'forward',
    fade: true,
  })
  const [breathingCfg, setBreathingCfg] = useState({
    speed: 6,
    color: '#50a0ff',
    min: 0,
    max: 255,
    curve: 'sine',
  })
  const [sparkleCfg, setSparkleCfg] = useState({
    speed: 10,
    color: '#ffffff',
    density: 25,
    fade: 24,
    mode: 'random',
    bg: '#000000',
  })

  const pattern = status?.pattern.type ?? 'rainbow'
  const bri = status?.pattern.global_brightness_pct ?? pendingBrightness
  const frame = status?.frame_ms ?? pendingFrame

  useEffect(() => {
    setPendingBrightness(bri)
    setPendingFrame(frame)
  }, [bri, frame])

  useEffect(() => {
    if (!status) return
    if (status.pattern.rainbow) setRainbowCfg(status.pattern.rainbow)
    if (status.pattern.chase) setChaseCfg(status.pattern.chase)
    if (status.pattern.breathing) setBreathingCfg(status.pattern.breathing)
    if (status.pattern.sparkle) setSparkleCfg(status.pattern.sparkle)
  }, [status])

  const patterns = useMemo(() => ['off', 'rainbow', 'chase', 'breathing', 'sparkle'], [])

  useEffect(() => {
    refresh()
    const t = setInterval(() => refresh(), 500)
    return () => clearInterval(t)
  }, [])

  async function applyPattern(next: string) {
    if (next === 'off') return setPattern('off')
    if (next === 'rainbow') return setRainbow(rainbowCfg)
    if (next === 'chase') return setChase(chaseCfg)
    if (next === 'breathing') return setBreathing(breathingCfg)
    if (next === 'sparkle') return setSparkle(sparkleCfg)
    return setPattern(next)
  }

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
              onChange={(e) => applyPattern((e.currentTarget as HTMLSelectElement).value)}
            >
              {patterns.map((p) => (
                <option value={p} key={p}>
                  {p}
                </option>
              ))}
            </select>
          </label>
        </div>

        {pattern === 'rainbow' ? (
          <div class="row">
            <div style="width: 100%">
              <h3>Rainbow</h3>
              <label>
                speed (RPM) {rainbowCfg.speed}
                <input
                  type="range"
                  min={0}
                  max={20}
                  value={rainbowCfg.speed}
                  onInput={(e) =>
                    setRainbowCfg((c) => ({
                      ...c,
                      speed: parseInt((e.currentTarget as HTMLInputElement).value, 10),
                    }))
                  }
                />
              </label>
              <label>
                sat {rainbowCfg.sat}%
                <input
                  type="range"
                  min={0}
                  max={100}
                  value={rainbowCfg.sat}
                  onInput={(e) =>
                    setRainbowCfg((c) => ({
                      ...c,
                      sat: parseInt((e.currentTarget as HTMLInputElement).value, 10),
                    }))
                  }
                />
              </label>
              <label>
                spread {rainbowCfg.spread} (1..50; tenths of 360°)
                <input
                  type="range"
                  min={1}
                  max={50}
                  value={rainbowCfg.spread}
                  onInput={(e) =>
                    setRainbowCfg((c) => ({
                      ...c,
                      spread: parseInt((e.currentTarget as HTMLInputElement).value, 10),
                    }))
                  }
                />
              </label>
              <button onClick={() => setRainbow(rainbowCfg)}>apply</button>
            </div>
          </div>
        ) : null}

        {pattern === 'chase' ? (
          <div class="row">
            <div style="width: 100%">
              <h3>Chase</h3>
              <label>
                speed (LEDs/sec)
                <input
                  type="number"
                  min={0}
                  max={255}
                  value={chaseCfg.speed}
                  onInput={(e) =>
                    setChaseCfg((c) => ({
                      ...c,
                      speed: parseInt((e.currentTarget as HTMLInputElement).value, 10),
                    }))
                  }
                />
              </label>
              <label>
                tail
                <input
                  type="number"
                  min={1}
                  max={255}
                  value={chaseCfg.tail}
                  onInput={(e) =>
                    setChaseCfg((c) => ({
                      ...c,
                      tail: parseInt((e.currentTarget as HTMLInputElement).value, 10),
                    }))
                  }
                />
              </label>
              <label>
                gap
                <input
                  type="number"
                  min={0}
                  max={255}
                  value={chaseCfg.gap}
                  onInput={(e) =>
                    setChaseCfg((c) => ({
                      ...c,
                      gap: parseInt((e.currentTarget as HTMLInputElement).value, 10),
                    }))
                  }
                />
              </label>
              <label>
                trains
                <input
                  type="number"
                  min={1}
                  max={255}
                  value={chaseCfg.trains}
                  onInput={(e) =>
                    setChaseCfg((c) => ({
                      ...c,
                      trains: parseInt((e.currentTarget as HTMLInputElement).value, 10),
                    }))
                  }
                />
              </label>
              <label>
                fg <input type="color" value={chaseCfg.fg} onInput={(e) => setChaseCfg((c) => ({ ...c, fg: (e.currentTarget as HTMLInputElement).value }))} />
              </label>
              <label>
                bg <input type="color" value={chaseCfg.bg} onInput={(e) => setChaseCfg((c) => ({ ...c, bg: (e.currentTarget as HTMLInputElement).value }))} />
              </label>
              <label>
                dir{' '}
                <select
                  value={chaseCfg.dir}
                  onChange={(e) =>
                    setChaseCfg((c) => ({
                      ...c,
                      dir: (e.currentTarget as HTMLSelectElement).value as any,
                    }))
                  }
                >
                  <option value="forward">forward</option>
                  <option value="reverse">reverse</option>
                  <option value="bounce">bounce</option>
                </select>
              </label>
              <label>
                fade{' '}
                <input
                  type="checkbox"
                  checked={chaseCfg.fade}
                  onChange={(e) => setChaseCfg((c) => ({ ...c, fade: (e.currentTarget as HTMLInputElement).checked }))}
                />
              </label>
              <button onClick={() => setChase(chaseCfg)}>apply</button>
            </div>
          </div>
        ) : null}

        {pattern === 'breathing' ? (
          <div class="row">
            <div style="width: 100%">
              <h3>Breathing</h3>
              <label>
                speed (breaths/min)
                <input
                  type="number"
                  min={0}
                  max={20}
                  value={breathingCfg.speed}
                  onInput={(e) =>
                    setBreathingCfg((c) => ({
                      ...c,
                      speed: parseInt((e.currentTarget as HTMLInputElement).value, 10),
                    }))
                  }
                />
              </label>
              <label>
                color <input type="color" value={breathingCfg.color} onInput={(e) => setBreathingCfg((c) => ({ ...c, color: (e.currentTarget as HTMLInputElement).value }))} />
              </label>
              <label>
                min
                <input
                  type="number"
                  min={0}
                  max={255}
                  value={breathingCfg.min}
                  onInput={(e) =>
                    setBreathingCfg((c) => ({
                      ...c,
                      min: parseInt((e.currentTarget as HTMLInputElement).value, 10),
                    }))
                  }
                />
              </label>
              <label>
                max
                <input
                  type="number"
                  min={0}
                  max={255}
                  value={breathingCfg.max}
                  onInput={(e) =>
                    setBreathingCfg((c) => ({
                      ...c,
                      max: parseInt((e.currentTarget as HTMLInputElement).value, 10),
                    }))
                  }
                />
              </label>
              <label>
                curve{' '}
                <select
                  value={breathingCfg.curve}
                  onChange={(e) =>
                    setBreathingCfg((c) => ({
                      ...c,
                      curve: (e.currentTarget as HTMLSelectElement).value as any,
                    }))
                  }
                >
                  <option value="sine">sine</option>
                  <option value="linear">linear</option>
                  <option value="ease">ease</option>
                </select>
              </label>
              <button onClick={() => setBreathing(breathingCfg)}>apply</button>
            </div>
          </div>
        ) : null}

        {pattern === 'sparkle' ? (
          <div class="row">
            <div style="width: 100%">
              <h3>Sparkle</h3>
              <label>
                speed (0..20)
                <input
                  type="number"
                  min={0}
                  max={20}
                  value={sparkleCfg.speed}
                  onInput={(e) =>
                    setSparkleCfg((c) => ({
                      ...c,
                      speed: parseInt((e.currentTarget as HTMLInputElement).value, 10),
                    }))
                  }
                />
              </label>
              <label>
                mode{' '}
                <select
                  value={sparkleCfg.mode}
                  onChange={(e) =>
                    setSparkleCfg((c) => ({
                      ...c,
                      mode: (e.currentTarget as HTMLSelectElement).value as any,
                    }))
                  }
                >
                  <option value="fixed">fixed</option>
                  <option value="random">random</option>
                  <option value="rainbow">rainbow</option>
                </select>
              </label>
              <label>
                color <input type="color" value={sparkleCfg.color} onInput={(e) => setSparkleCfg((c) => ({ ...c, color: (e.currentTarget as HTMLInputElement).value }))} />
              </label>
              <label>
                density (%)
                <input
                  type="number"
                  min={0}
                  max={100}
                  value={sparkleCfg.density}
                  onInput={(e) =>
                    setSparkleCfg((c) => ({
                      ...c,
                      density: parseInt((e.currentTarget as HTMLInputElement).value, 10),
                    }))
                  }
                />
              </label>
              <label>
                fade (1..255)
                <input
                  type="number"
                  min={1}
                  max={255}
                  value={sparkleCfg.fade}
                  onInput={(e) =>
                    setSparkleCfg((c) => ({
                      ...c,
                      fade: parseInt((e.currentTarget as HTMLInputElement).value, 10),
                    }))
                  }
                />
              </label>
              <label>
                bg <input type="color" value={sparkleCfg.bg} onInput={(e) => setSparkleCfg((c) => ({ ...c, bg: (e.currentTarget as HTMLInputElement).value }))} />
              </label>
              <button onClick={() => setSparkle(sparkleCfg)}>apply</button>
            </div>
          </div>
        ) : null}

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
              onInput={(e) =>
                setPendingFrame(parseInt((e.currentTarget as HTMLInputElement).value, 10))
              }
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
