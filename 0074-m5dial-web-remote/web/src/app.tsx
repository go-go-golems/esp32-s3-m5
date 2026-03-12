import { useEffect, useMemo, useRef, useState } from 'react'
import { useStore, STATIONS, type StationType } from './store'

// ─── CRT overlays ───────────────────────────────────────────

function Scanlines() {
  return <div className="scanlines" />
}

function CRTOverlay() {
  return <div className="crt-overlay" />
}

// ─── Glitch text ────────────────────────────────────────────

function GlitchText({ children }: { children: string }) {
  const [glitch, setGlitch] = useState(false)
  useEffect(() => {
    let timeout: number
    const run = () => {
      const delay = 2000 + Math.random() * 6000
      timeout = window.setTimeout(() => {
        setGlitch(true)
        window.setTimeout(() => { setGlitch(false); run() }, 80 + Math.random() * 120)
      }, delay)
    }
    run()
    return () => clearTimeout(timeout)
  }, [])

  if (!glitch) return <span className="glitch">{children}</span>
  const chars = '░▒▓█╳╬╫╪┼'
  const text = children
  const start = Math.floor(Math.random() * text.length)
  const len = Math.floor(Math.random() * 4) + 1
  const glitched = text.slice(0, start) + Array.from({ length: len }, () => chars[Math.floor(Math.random() * chars.length)]).join('') + text.slice(start + len)
  return <span className="glitch active">{glitched}</span>
}

// ─── Cursor ─────────────────────────────────────────────────

function Cursor() {
  return <span className="cursor">{'\u2588'}</span>
}

// ─── Ambient noise strip ────────────────────────────────────

function AmbientNoise() {
  const [noise, setNoise] = useState('')
  useEffect(() => {
    const iv = setInterval(() => {
      const chars = '░ ▒ ▓'
      setNoise(Array.from({ length: 60 }, () => chars[Math.floor(Math.random() * chars.length)]).join(''))
    }, 100)
    return () => clearInterval(iv)
  }, [])
  return <div className="ambient-noise">{noise}</div>
}

// ─── Waveform visualization ─────────────────────────────────

function Waveform({ active }: { active: boolean }) {
  const [frame, setFrame] = useState(0)
  useEffect(() => {
    if (!active) return
    const iv = setInterval(() => setFrame(f => f + 1), 150)
    return () => clearInterval(iv)
  }, [active])

  if (!active) return <div className="waveform" style={{ opacity: 0.3 }}>{'─'.repeat(28)}</div>

  const patterns = [
    '─┐    ┌─┐       ┌┐        ',
    '  │  ┌┘ └┐   ┌┐ ││    ┌─  ',
    '  └──┘   └┐ ┌┘│ │└┐  ┌┘   ',
    '          └─┘ └─┘ └──┘    ',
  ]
  const shift = frame % 8
  return (
    <div className="waveform">
      {patterns.map((line, i) => (
        <div key={i}>{line.slice(shift) + line.slice(0, shift)}</div>
      ))}
    </div>
  )
}

// ─── Spectrogram visualization ──────────────────────────────

function Spectrogram({ active }: { active: boolean }) {
  const [frame, setFrame] = useState(0)
  useEffect(() => {
    if (!active) return
    const iv = setInterval(() => setFrame(f => f + 1), 200)
    return () => clearInterval(iv)
  }, [active])

  const freqs = ['8kHz', '4kHz', '2kHz', '1kHz', ' low']
  return (
    <div className="spectrogram">
      {freqs.map((f, fi) => {
        const density = (freqs.length - fi) * 0.15 + 0.1
        const line = active
          ? Array.from({ length: 28 }, (_, i) => {
              const val = Math.sin(i * 0.5 + frame * 0.3 + fi) * 0.5 + 0.5
              return val > (1 - density - fi * 0.15) ? '█' : val > 0.6 ? '░' : '·'
            }).join(' ')
          : Array.from({ length: 28 }, () => '·').join(' ')
        return <div key={f}><span className="freq-label">{f}</span>  {line}</div>
      })}
    </div>
  )
}

// ─── SVG Dial Mirror ────────────────────────────────────────

const NUM_STATIONS = 120
const ARC_START = 135
const ARC_SPAN = 270

function polarToXY(cx: number, cy: number, r: number, angleDeg: number): [number, number] {
  const rad = (angleDeg * Math.PI) / 180
  return [cx + r * Math.cos(rad), cy + r * Math.sin(rad)]
}

function arcPath(cx: number, cy: number, r: number, startDeg: number, endDeg: number): string {
  const [x1, y1] = polarToXY(cx, cy, r, startDeg)
  const [x2, y2] = polarToXY(cx, cy, r, endDeg)
  const sweep = endDeg - startDeg
  const largeArc = sweep > 180 ? 1 : 0
  return `M ${x1} ${y1} A ${r} ${r} 0 ${largeArc} 1 ${x2} ${y2}`
}

function stationColor(type: StationType): string {
  switch (type) {
    case 'clear': return '#7fff7f'
    case 'static': return '#ffff60'
    case 'hidden': return '#ff6060'
    case 'distorted': return '#ffa040'
    default: return 'rgba(200,230,192,0.08)'
  }
}

function DialMirror({ frequency, band, locked }: { frequency: number; band: string; locked: boolean }) {
  const cx = 130, cy = 130, outerR = 118, innerR = 110, dotR = 95

  const posAngle = ARC_START + (frequency / (NUM_STATIONS - 1)) * ARC_SPAN

  // Station dot data
  const stationDots = useMemo(() => {
    const dots: { x: number; y: number; color: string; r: number }[] = []
    const stationMap = new Map(STATIONS.map(s => [s.pos, s]))
    for (let i = 0; i < NUM_STATIONS; i++) {
      const angle = ARC_START + (i / (NUM_STATIONS - 1)) * ARC_SPAN
      const [x, y] = polarToXY(cx, cy, dotR, angle)
      const s = stationMap.get(i)
      dots.push({
        x, y,
        color: s ? stationColor(s.type) : 'rgba(200,230,192,0.06)',
        r: s ? 2.5 : 1,
      })
    }
    return dots
  }, [])

  const [handX, handY] = polarToXY(cx, cy, (outerR + innerR) / 2, posAngle)

  // Sweep arc path (from start to current position)
  const sweepEnd = ARC_START + (frequency / (NUM_STATIONS - 1)) * ARC_SPAN

  return (
    <div className="dial-mirror">
      <svg viewBox="0 0 260 260">
        {/* Background arc */}
        <path
          d={arcPath(cx, cy, (outerR + innerR) / 2, ARC_START, ARC_START + ARC_SPAN)}
          fill="none"
          stroke="rgba(200,230,192,0.12)"
          strokeWidth={outerR - innerR}
          strokeLinecap="round"
        />

        {/* Sweep arc */}
        {frequency > 0 && (
          <path
            d={arcPath(cx, cy, (outerR + innerR) / 2, ARC_START, sweepEnd)}
            fill="none"
            stroke="rgba(0,255,0,0.3)"
            strokeWidth={outerR - innerR}
            strokeLinecap="round"
          />
        )}

        {/* Station dots */}
        {stationDots.map((dot, i) => (
          <circle
            key={i}
            cx={dot.x}
            cy={dot.y}
            r={i === frequency ? 4 : dot.r}
            fill={i === frequency ? '#ffffff' : dot.color}
          />
        ))}

        {/* Position indicator */}
        <circle cx={handX} cy={handY} r={6} fill="#ffffff" opacity={0.9} />
        <circle cx={handX} cy={handY} r={4} fill="#7fff7f" />

        {/* Band label */}
        <text x={cx} y={38} textAnchor="middle" fill="rgba(200,230,192,0.5)" fontSize="11" fontFamily="'Courier New', monospace">{band}</text>

        {/* Lock indicator */}
        {locked && (
          <text x={cx} y={cy + 36} textAnchor="middle" fill="#7fff7f" fontSize="10" fontFamily="'Courier New', monospace">◉ LOCKED</text>
        )}

        {/* Center ring */}
        <circle cx={cx} cy={cy} r={68} fill="none" stroke="rgba(200,230,192,0.06)" strokeWidth={1} />
      </svg>
    </div>
  )
}

// ─── Connect / device selection screen ──────────────────────

function ConnectScreen() {
  const wsConnected = useStore(s => s.wsConnected)
  const connectWs = useStore(s => s.connectWs)
  const devices = useStore(s => s.devices)
  const selectedDeviceId = useStore(s => s.selectedDeviceId)
  const selectDevice = useStore(s => s.selectDevice)
  const initRadioMode = useStore(s => s.initRadioMode)

  useEffect(() => { connectWs() }, [connectWs])

  const canStart = wsConnected && selectedDeviceId && devices.some(d => d.device_id === selectedDeviceId && d.connected)

  return (
    <div className="connect-screen">
      <pre className="boot-text">
        {'NAVI_OS v7.02a\nLAYER:07\n\nNARRATIVE RADIO'}
      </pre>

      <div style={{ opacity: wsConnected ? 1 : 0.4 }}>
        {wsConnected ? '◉ connected to wired' : '○ connecting to wired...'}
      </div>

      {devices.length > 0 && (
        <>
          <div style={{ opacity: 0.4 }}>select device:</div>
          <div className="device-picker">
            {devices.map(d => (
              <button
                key={d.device_id}
                className={`device-btn ${selectedDeviceId === d.device_id ? 'selected' : ''}`}
                onClick={() => selectDevice(d.device_id)}
              >
                {d.device_id} {d.connected ? '◉' : '○'}
              </button>
            ))}
          </div>
        </>
      )}

      {devices.length === 0 && wsConnected && (
        <div style={{ opacity: 0.3 }}>waiting for device . . .<Cursor /></div>
      )}

      {canStart && (
        <span className="nav-btn" onClick={initRadioMode}>
          [enter the wired]
        </span>
      )}

      <div className="whisper">
        no matter where you go<br />
        everyone is connected.
      </div>
    </div>
  )
}

// ─── Main Radio Screen ──────────────────────────────────────

function RadioScreen() {
  const radio = useStore(s => s.radio)
  const selectedDeviceId = useStore(s => s.selectedDeviceId)
  const devices = useStore(s => s.devices)
  const device = useMemo(
    () => devices.find(d => d.device_id === selectedDeviceId),
    [devices, selectedDeviceId]
  )

  const hasSignal = radio.stationType !== 'empty'
  const logRef = useRef<HTMLDivElement>(null)

  return (
    <div className="shell" style={{ animation: 'fadeIn 1s ease' }}>
      {/* Header */}
      <div className="header">
        <span><GlitchText>NAVI_OS v7.02a</GlitchText></span>
        <span>dev: {selectedDeviceId || '?'}</span>
        <span>RADIO</span>
        <span>LAYER:07</span>
      </div>

      {/* Status box */}
      <div className="status-box">
        <div>
          <span className="label">BAND: </span>
          <span className="value">{radio.band}</span>
          {'          '}
          <span className="label">FREQ: </span>
          <span className="value">{radio.freqLabel} MHz</span>
        </div>
        <div>
          <span className="label">STATION: </span>
          <span className={`value ${radio.stationType === 'hidden' ? 'danger' : radio.stationType === 'static' ? 'warn' : ''}`}>
            {radio.stationName}
          </span>
        </div>
        <div>
          <span className="label">STATUS: </span>
          <span className="value">{radio.locked ? '◉ LOCKED' : '○ scanning'}</span>
          {'      '}
          <span className="label">signal: </span>
          <span className="signal-bar">
            {'█'.repeat(Math.round(radio.signalStrength / 5))}
            {'░'.repeat(20 - Math.round(radio.signalStrength / 5))}
            {' '}{radio.signalStrength}%
          </span>
        </div>
      </div>

      {/* Dial mirror + info section */}
      <div className="dial-section">
        <DialMirror
          frequency={radio.frequency}
          band={radio.band}
          locked={radio.locked}
        />
        <div className="dial-info">
          <div className="viz-section">
            <div className="viz-label">waveform:</div>
            <Waveform active={hasSignal} />
          </div>

          <div className="viz-section">
            <div className="viz-label">spectral analysis:</div>
            <Spectrogram active={hasSignal} />
          </div>
        </div>
      </div>

      {/* Hidden text reveal */}
      {radio.revealInProgress && radio.revealedTexts.length > 0 && (
        <div className="reveal-box">
          <div className="reveal-label">embedded text found in noise floor:</div>
          <RevealText lines={radio.revealedTexts} />
        </div>
      )}

      {/* Station log */}
      <div className="station-log">
        <div className="log-label">station log:</div>
        <div ref={logRef}>
          {radio.stationLog.slice(0, 12).map((entry, i) => (
            <div key={i} className={`log-entry ${entry.action === 'locked' ? 'locked' : entry.action.includes('reveal') ? 'revealed' : ''}`}>
              <span className="log-time">{entry.time}</span>
              <span className="log-freq">{entry.freq}</span>
              <span className="log-name">{entry.name}</span>
              <span className="log-action">{entry.action}</span>
            </div>
          ))}
          {radio.stationLog.length === 0 && (
            <div style={{ opacity: 0.3 }}>twist the dial to begin scanning . . .</div>
          )}
        </div>
      </div>

      {/* Device state (compact) */}
      {device && (
        <div style={{ opacity: 0.3, fontSize: '11px', lineHeight: 1.8 }}>
          pos:{device.position} seq:{device.last_seq} rx:{device.rx_count} {device.connected ? 'online' : 'offline'}
        </div>
      )}

      <AmbientNoise />

      <div className="whisper">
        close the world<br />
        open the nEXt
      </div>
    </div>
  )
}

// ─── Typewriter reveal ──────────────────────────────────────

function RevealText({ lines }: { lines: string[] }) {
  const [revealed, setRevealed] = useState(0)
  useEffect(() => {
    setRevealed(0)
    const iv = setInterval(() => {
      setRevealed(r => {
        if (r >= lines.length) { clearInterval(iv); return r }
        return r + 1
      })
    }, 600)
    return () => clearInterval(iv)
  }, [lines])

  return (
    <div className="reveal-text">
      {lines.slice(0, revealed).map((line, i) => (
        <div key={i} style={{ animation: 'fadeIn 0.5s ease' }}>
          {line.split('').join(' ')}
        </div>
      ))}
      {revealed < lines.length && <Cursor />}
    </div>
  )
}

// ─── Root App ───────────────────────────────────────────────

export function App() {
  const radioActive = useStore(s => s.radio.active)

  return (
    <div>
      <Scanlines />
      <CRTOverlay />
      {radioActive ? <RadioScreen /> : <ConnectScreen />}
    </div>
  )
}
