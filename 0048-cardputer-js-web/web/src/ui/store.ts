import { create } from 'zustand'

type EvalResponse = {
  ok: boolean
  output?: string
  error?: string | null
  timed_out?: boolean
}

type EncoderMsg = {
  type: 'encoder'
  seq: number
  ts_ms: number
  pos: number
  delta: number
  pressed: boolean
}

type Store = {
  code: string
  running: boolean
  last: EvalResponse | null
  setCode: (code: string) => void
  run: () => Promise<void>
  wsConnected: boolean
  encoder: EncoderMsg | null
  connectWs: () => void
}

let ws: WebSocket | null = null
let reconnectTimer: number | null = null
let reconnectBackoffMs = 250

export const useStore = create<Store>((set, get) => ({
  code: '1+1',
  running: false,
  last: null,
  wsConnected: false,
  encoder: null,
  setCode: (code) => set({ code }),
  run: async () => {
    set({ running: true })
    try {
      const res = await fetch('/api/js/eval', {
        method: 'POST',
        headers: { 'Content-Type': 'text/plain; charset=utf-8' },
        body: get().code,
      })
      const json = (await res.json()) as EvalResponse
      set({ last: json })
    } catch (e: any) {
      set({ last: { ok: false, error: String(e) } })
    } finally {
      set({ running: false })
    }
  },
  connectWs: () => {
    if (ws && (ws.readyState === WebSocket.OPEN || ws.readyState === WebSocket.CONNECTING)) return
    if (reconnectTimer != null) {
      window.clearTimeout(reconnectTimer)
      reconnectTimer = null
    }

    const proto = window.location.protocol === 'https:' ? 'wss' : 'ws'
    const url = `${proto}://${window.location.host}/ws`

    ws = new WebSocket(url)

    ws.onopen = () => {
      reconnectBackoffMs = 250
      set({ wsConnected: true })
    }

    ws.onclose = () => {
      set({ wsConnected: false })
      if (reconnectTimer == null) {
        reconnectTimer = window.setTimeout(() => {
          reconnectTimer = null
          get().connectWs()
        }, reconnectBackoffMs)
        reconnectBackoffMs = Math.min(5000, reconnectBackoffMs * 2)
      }
    }

    ws.onerror = () => {
      // onclose will schedule reconnect; no-op here.
    }

    ws.onmessage = (ev) => {
      try {
        const msg = JSON.parse(ev.data) as EncoderMsg
        if (msg && msg.type === 'encoder') {
          set({ encoder: msg })
        }
      } catch {
        // ignore malformed frames
      }
    }
  },
}))
