import { create } from 'zustand'

// ─── Station data ──────────────────────────────────────────

export type StationType = 'empty' | 'clear' | 'static' | 'hidden' | 'distorted'
export const stationTypeCode: Record<StationType, number> = { empty: 0, clear: 1, static: 2, hidden: 3, distorted: 4 }

export type StationDef = {
  pos: number
  type: StationType
  name: string
  freq: string
  hiddenText?: string[]
  dwellMs?: number
}

export const STATIONS: StationDef[] = [
  { pos: 5,   type: 'clear',     name: 'phantom_relay',  freq: '89.3' },
  { pos: 12,  type: 'hidden',    name: '???',            freq: '91.0', hiddenText: ['close the world', 'open the nEXt'], dwellMs: 3000 },
  { pos: 20,  type: 'clear',     name: 'wire_temple',    freq: '93.5' },
  { pos: 28,  type: 'static',    name: 'STATIC',         freq: '95.7' },
  { pos: 35,  type: 'clear',     name: 'navi_broadcast', freq: '97.5' },
  { pos: 42,  type: 'distorted', name: 'tetsuo_echo',    freq: '99.8' },
  { pos: 50,  type: 'clear',     name: 'layer_07_hub',   freq: '102.0' },
  { pos: 55,  type: 'static',    name: 'STATIC',         freq: '103.5' },
  { pos: 60,  type: 'hidden',    name: '???',            freq: '105.0', hiddenText: ['help me', 'i am still here', 'they did not let', 'me go'], dwellMs: 4000 },
  { pos: 67,  type: 'clear',     name: 'accela_wave',    freq: '107.0' },
  { pos: 73,  type: 'clear',     name: 'cyberia_fm',     freq: '108.8' },
  { pos: 80,  type: 'distorted', name: 'eiri_signal',    freq: '110.5' },
  { pos: 88,  type: 'clear',     name: 'protocol_7',     freq: '112.8' },
  { pos: 95,  type: 'static',    name: 'STATIC',         freq: '115.0' },
  { pos: 100, type: 'hidden',    name: '???',            freq: '116.5', hiddenText: ['do you understand', 'we share the same signal', 'you cannot cut me off'], dwellMs: 3500 },
  { pos: 107, type: 'clear',     name: 'knights_freq',   freq: '118.0' },
  { pos: 115, type: 'clear',     name: 'serial_exp',     freq: '119.8' },
]

export function stationAt(pos: number): StationDef | undefined {
  return STATIONS.find(s => s.pos === pos)
}

function freqLabel(pos: number): string {
  const s = stationAt(pos)
  if (s) return s.freq
  const mhz = 88 + (pos * 30) / 119
  return mhz.toFixed(1)
}

function stationName(pos: number): string {
  const s = stationAt(pos)
  return s ? s.name : '---'
}

function stationTypeAt(pos: number): StationType {
  const s = stationAt(pos)
  return s ? s.type : 'empty'
}

// ─── Types ──────────────────────────────────────────────────

export type DeviceState = {
  device_id: string
  connected: boolean
  remote_addr: string
  last_type: string
  last_button?: string
  last_seq: number
  position: number
  last_delta: number
  last_swipe?: string
  last_text?: string
  last_command?: string
  last_command_status?: string
  last_request_id?: number
  last_seen: string
  rx_count: number
}

export type HistoryEntry = {
  id: number
  received_at: string
  device_id: string
  type: string
  raw: string
}

type ServerSnapshot = {
  type: 'server_snapshot' | 'server_status'
  generated_at: string
  devices: DeviceState[]
  history: HistoryEntry[]
}

type UiCommandResultFrame = {
  type: 'ui_command_result'
  generated_at: string
  device_id: string
  command: string
  request_id: number
  status: string
  reason?: string
}

type DeviceEventFrame = {
  type: 'device_event'
  device_id: string
  payload?: {
    type?: string
    request_id?: number
    command?: string
    status?: string
    text?: string
    pos?: number
    delta?: number
    direction?: string
    kind?: string
  }
}

export type StationLogEntry = {
  time: string
  freq: string
  name: string
  action: string
}

export type CommandFeedback = {
  requestId: number
  deviceId: string
  command: string
  status: string
  detail: string
  atMs: number
}

type CommandPayload = { text?: string; value?: number }

// ─── Radio state ────────────────────────────────────────────

export type RadioState = {
  active: boolean
  frequency: number
  band: string
  locked: boolean
  stationType: StationType
  stationName: string
  freqLabel: string
  revealedTexts: string[]
  revealInProgress: boolean
  dwellStartMs: number
  stationLog: StationLogEntry[]
  signalStrength: number
}

// ─── Store ──────────────────────────────────────────────────

type Store = {
  wsConnected: boolean
  devices: DeviceState[]
  history: HistoryEntry[]
  selectedDeviceId: string
  lastCommandFeedback: CommandFeedback | null
  radio: RadioState
  connectWs: () => void
  selectDevice: (id: string) => void
  sendUiCommand: (deviceId: string, command: string, payload?: CommandPayload) => void
  initRadioMode: () => void
}

let socket: WebSocket | null = null
let reconnectTimer: number | null = null
let reconnectBackoffMs = 300
let nextRequestID = 1

function ensureSelectedDevice(devices: DeviceState[], selectedDeviceId: string) {
  if (selectedDeviceId && devices.some(d => d.device_id === selectedDeviceId)) return selectedDeviceId
  return devices[0]?.device_id ?? ''
}

function setCommandFeedback(requestId: number, deviceId: string, command: string, status: string, detail: string): Pick<Store, 'lastCommandFeedback'> {
  return { lastCommandFeedback: { requestId, deviceId, command, status, detail, atMs: Date.now() } }
}

function addLogEntry(log: StationLogEntry[], freq: string, name: string, action: string): StationLogEntry[] {
  const entry: StationLogEntry = { time: new Date().toLocaleTimeString(), freq, name, action }
  return [entry, ...log].slice(0, 50)
}

function initialRadio(): RadioState {
  return {
    active: false,
    frequency: 0,
    band: 'WIRED',
    locked: false,
    stationType: 'empty',
    stationName: '---',
    freqLabel: '88.0',
    revealedTexts: [],
    revealInProgress: false,
    dwellStartMs: 0,
    stationLog: [],
    signalStrength: 0,
  }
}

// ─── Audio engine ───────────────────────────────────────────

let audioCtx: AudioContext | null = null
let currentGain: GainNode | null = null
let currentOsc: OscillatorNode | null = null
let currentNoise: AudioBufferSourceNode | null = null

function ensureAudio() {
  if (!audioCtx) audioCtx = new AudioContext()
  if (audioCtx.state === 'suspended') audioCtx.resume()
}

function stopAudio() {
  try { currentOsc?.stop() } catch { /* ok */ }
  try { currentNoise?.stop() } catch { /* ok */ }
  currentOsc = null
  currentNoise = null
  currentGain = null
}

function playStation(type: StationType) {
  ensureAudio()
  stopAudio()
  if (!audioCtx || type === 'empty') return

  currentGain = audioCtx.createGain()
  currentGain.gain.value = 0
  currentGain.connect(audioCtx.destination)
  // Fade in
  currentGain.gain.linearRampToValueAtTime(type === 'clear' ? 0.08 : type === 'static' ? 0.04 : 0.06, audioCtx.currentTime + 0.2)

  if (type === 'clear') {
    const osc = audioCtx.createOscillator()
    osc.type = 'sine'
    osc.frequency.value = 80 + Math.random() * 40
    const lfo = audioCtx.createOscillator()
    lfo.frequency.value = 0.3
    const lfoGain = audioCtx.createGain()
    lfoGain.gain.value = 10
    lfo.connect(lfoGain)
    lfoGain.connect(osc.frequency)
    lfo.start()
    osc.connect(currentGain)
    osc.start()
    currentOsc = osc
  } else if (type === 'static' || type === 'distorted') {
    const bufSize = audioCtx.sampleRate * 2
    const buf = audioCtx.createBuffer(1, bufSize, audioCtx.sampleRate)
    const data = buf.getChannelData(0)
    for (let i = 0; i < bufSize; i++) data[i] = Math.random() * 2 - 1
    const src = audioCtx.createBufferSource()
    src.buffer = buf
    src.loop = true
    const filter = audioCtx.createBiquadFilter()
    filter.type = type === 'distorted' ? 'lowpass' : 'bandpass'
    filter.frequency.value = type === 'distorted' ? 400 : 2000
    filter.Q.value = type === 'distorted' ? 5 : 1
    src.connect(filter)
    filter.connect(currentGain)
    src.start()
    currentNoise = src
  } else if (type === 'hidden') {
    const osc1 = audioCtx.createOscillator()
    osc1.type = 'sawtooth'
    osc1.frequency.value = 55
    const osc2 = audioCtx.createOscillator()
    osc2.type = 'sawtooth'
    osc2.frequency.value = 55.5
    const merge = audioCtx.createGain()
    merge.gain.value = 0.5
    osc1.connect(merge)
    osc2.connect(merge)
    merge.connect(currentGain)
    osc1.start()
    osc2.start()
    currentOsc = osc1
  }
}

// ─── Dwell timer for hidden stations ────────────────────────

let dwellTimer: number | null = null

function startDwell(store: ReturnType<typeof useStore.getState>) {
  if (dwellTimer) clearTimeout(dwellTimer)
  const station = stationAt(store.radio.frequency)
  if (!station || station.type !== 'hidden' || !station.hiddenText) return

  store.radio.dwellStartMs = Date.now()

  dwellTimer = window.setTimeout(() => {
    const s = useStore.getState()
    if (s.radio.frequency !== station.pos) return // moved away

    useStore.setState({
      radio: {
        ...s.radio,
        revealedTexts: station.hiddenText!,
        revealInProgress: true,
        stationLog: addLogEntry(s.radio.stationLog, station.freq, station.name, 'text revealed'),
      }
    })

    // Send reveal to dial
    if (s.selectedDeviceId && station.hiddenText!.length > 0) {
      s.sendUiCommand(s.selectedDeviceId, 'show_reveal', { text: station.hiddenText![0] })
    }
  }, station.dwellMs ?? 3000)
}

function cancelDwell() {
  if (dwellTimer) { clearTimeout(dwellTimer); dwellTimer = null }
}

// ─── Create store ───────────────────────────────────────────

export const useStore = create<Store>((set, get) => ({
  wsConnected: false,
  devices: [],
  history: [],
  selectedDeviceId: '',
  lastCommandFeedback: null,
  radio: initialRadio(),

  connectWs: () => {
    if (socket && (socket.readyState === WebSocket.OPEN || socket.readyState === WebSocket.CONNECTING)) return
    if (reconnectTimer !== null) { window.clearTimeout(reconnectTimer); reconnectTimer = null }

    const proto = window.location.protocol === 'https:' ? 'wss' : 'ws'
    socket = new WebSocket(`${proto}://${window.location.host}/ws/browser`)

    socket.onopen = () => {
      reconnectBackoffMs = 300
      set({ wsConnected: true })
      // Re-init radio mode if it was active
      const s = get()
      if (s.radio.active && s.selectedDeviceId) {
        s.initRadioMode()
      }
    }

    socket.onclose = () => {
      set({ wsConnected: false })
      if (reconnectTimer === null) {
        reconnectTimer = window.setTimeout(() => { reconnectTimer = null; get().connectWs() }, reconnectBackoffMs)
        reconnectBackoffMs = Math.min(5000, reconnectBackoffMs * 2)
      }
    }

    socket.onerror = () => {}

    socket.onmessage = (event) => {
      const raw = typeof event.data === 'string' ? event.data : ''
      let parsed: unknown = null
      try { parsed = JSON.parse(raw) } catch { return }
      if (!parsed || typeof parsed !== 'object') return

      const msg = parsed as { type?: string }

      if (msg.type === 'server_snapshot' || msg.type === 'server_status') {
        const snapshot = parsed as ServerSnapshot
        set(state => ({
          devices: snapshot.devices,
          history: snapshot.history,
          selectedDeviceId: ensureSelectedDevice(snapshot.devices, state.selectedDeviceId),
        }))
        return
      }

      if (msg.type === 'ui_command_result') {
        const result = parsed as UiCommandResultFrame
        set(setCommandFeedback(result.request_id, result.device_id, result.command, result.status, result.reason ?? 'queued'))
        return
      }

      if (msg.type === 'device_event') {
        const de = parsed as DeviceEventFrame
        const p = de.payload
        if (!p) return

        // Handle encoder events for radio
        if (p.type === 'encoder' && get().radio.active) {
          const pos = p.pos ?? 0
          const st = stationTypeAt(pos)
          const name = stationName(pos)
          const freq = freqLabel(pos)
          const prev = get().radio

          playStation(st)

          const log = pos !== prev.frequency
            ? addLogEntry(prev.stationLog, freq, name, st === 'empty' ? 'empty' : `tuned`)
            : prev.stationLog

          cancelDwell()
          set({
            radio: {
              ...prev,
              frequency: pos,
              stationType: st,
              stationName: name,
              freqLabel: freq,
              stationLog: log,
              revealedTexts: pos !== prev.frequency ? [] : prev.revealedTexts,
              revealInProgress: pos !== prev.frequency ? false : prev.revealInProgress,
              signalStrength: st === 'clear' ? 72 + Math.floor(Math.random() * 28) : st === 'static' ? 20 + Math.floor(Math.random() * 30) : st === 'hidden' ? 45 + Math.floor(Math.random() * 20) : st === 'distorted' ? 10 + Math.floor(Math.random() * 15) : 0,
            }
          })

          if (st === 'hidden') startDwell(get())
          return
        }

        // Handle swipe for band changes
        if (p.type === 'swipe' && get().radio.active) {
          const dir = p.direction
          if (dir === 'left' || dir === 'right') {
            const bands = ['AM', 'FM', 'WIRED'] as const
            const prev = get().radio
            const idx = bands.indexOf(prev.band as typeof bands[number])
            const next = dir === 'right' ? bands[(idx + 1) % 3] : bands[(idx + 2) % 3]
            set({
              radio: {
                ...prev,
                band: next,
                stationLog: addLogEntry(prev.stationLog, prev.freqLabel, '', `band → ${next}`),
              }
            })
          }
          return
        }

        // Handle button for lock toggle
        if (p.type === 'button' && p.kind === 'short' && get().radio.active) {
          const prev = get().radio
          const nowLocked = !prev.locked
          set({
            radio: {
              ...prev,
              locked: nowLocked,
              stationLog: addLogEntry(prev.stationLog, prev.freqLabel, prev.stationName, nowLocked ? 'locked' : 'unlocked'),
            }
          })
          return
        }

        // Handle ui_command_ack
        if (p.type === 'ui_command_ack') {
          set(setCommandFeedback(p.request_id ?? 0, de.device_id, p.command ?? 'unknown', `ack:${p.status ?? 'unknown'}`, p.text ?? ''))
          return
        }
      }
    }
  },

  selectDevice: (id) => set({ selectedDeviceId: id }),

  sendUiCommand: (deviceId, command, payload = {}) => {
    const requestId = nextRequestID++
    if (!deviceId) { set(setCommandFeedback(requestId, '', command, 'rejected', 'no device')); return }
    if (!socket || socket.readyState !== WebSocket.OPEN) { set(setCommandFeedback(requestId, deviceId, command, 'rejected', 'not connected')); return }

    const frame = {
      type: 'ui_command',
      device_id: deviceId,
      command,
      request_id: requestId,
      ...(payload.text !== undefined ? { text: payload.text } : {}),
      ...(payload.value !== undefined ? { value: payload.value } : {}),
    }

    try {
      socket.send(JSON.stringify(frame))
      set(setCommandFeedback(requestId, deviceId, command, 'sent', 'sent'))
    } catch (error) {
      set(setCommandFeedback(requestId, deviceId, command, 'rejected', String(error)))
    }
  },

  initRadioMode: () => {
    const s = get()
    const deviceId = s.selectedDeviceId
    if (!deviceId) return

    // 1. Set mode to radio
    s.sendUiCommand(deviceId, 'set_mode', { value: 1 })

    // 2. Send all station definitions
    for (const station of STATIONS) {
      s.sendUiCommand(deviceId, 'set_station', {
        value: station.pos,
        text: `${stationTypeCode[station.type]}:${station.name}`,
      })
    }

    set({
      radio: {
        ...initialRadio(),
        active: true,
        band: 'WIRED',
      }
    })
  },
}))
