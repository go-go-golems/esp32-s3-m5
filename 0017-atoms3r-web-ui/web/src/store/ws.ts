import { create } from 'zustand'

type ButtonEvent = {
  type: 'button'
  ts_ms: number
}

type WsState = {
  connected: boolean
  lastError: string | null
  buttonEvents: ButtonEvent[]
  terminalText: string
  connect: () => void
  disconnect: () => void
  sendBytes: (bytes: Uint8Array) => void
  clearTerminal: () => void
}

let ws: WebSocket | null = null
let decoder = new TextDecoder('utf-8')
let reconnectTimer: number | null = null
let shouldReconnect = true
let reconnectAttempt = 0

function wsUrl(): string {
  const proto = window.location.protocol === 'https:' ? 'wss:' : 'ws:'
  return `${proto}//${window.location.host}/ws`
}

function clearReconnectTimer() {
  if (reconnectTimer !== null) {
    window.clearTimeout(reconnectTimer)
    reconnectTimer = null
  }
}

export const useWsStore = create<WsState>((set) => ({
  connected: false,
  lastError: null,
  buttonEvents: [],
  terminalText: '',

  connect: () => {
    if (ws && (ws.readyState === WebSocket.OPEN || ws.readyState === WebSocket.CONNECTING)) {
      return
    }
    shouldReconnect = true
    clearReconnectTimer()

    ws = new WebSocket(wsUrl())
    ws.binaryType = 'arraybuffer'

    ws.onopen = () => {
      reconnectAttempt = 0
      set({ connected: true, lastError: null })
    }
    ws.onclose = () => {
      set({ connected: false })
      ws = null
      if (!shouldReconnect) return

      // Basic backoff reconnect (max ~5s).
      reconnectAttempt++
      const delayMs = Math.min(5000, 250 * Math.pow(2, reconnectAttempt - 1))
      reconnectTimer = window.setTimeout(() => {
        reconnectTimer = null
        useWsStore.getState().connect()
      }, delayMs)
    }
    ws.onerror = () => set({ lastError: 'WebSocket error' })
    ws.onmessage = (ev) => {
      if (typeof ev.data === 'string') {
        try {
          const obj = JSON.parse(ev.data)
          if (obj && obj.type === 'button' && typeof obj.ts_ms === 'number') {
            set((s) => ({ buttonEvents: [...s.buttonEvents, obj as ButtonEvent] }))
          }
        } catch {
          // ignore
        }
        return
      }

      // Binary UART RX
      const bytes = new Uint8Array(ev.data as ArrayBuffer)
      const chunk = decoder.decode(bytes, { stream: true })
      if (chunk.length > 0) {
        set((s) => ({ terminalText: s.terminalText + chunk }))
      }
    }
  },

  disconnect: () => {
    shouldReconnect = false
    clearReconnectTimer()
    if (ws) {
      ws.close()
      ws = null
    }
    set({ connected: false })
  },

  sendBytes: (bytes: Uint8Array) => {
    if (!ws || ws.readyState !== WebSocket.OPEN) return
    ws.send(bytes)
  },

  clearTerminal: () => set({ terminalText: '' }),
}))


