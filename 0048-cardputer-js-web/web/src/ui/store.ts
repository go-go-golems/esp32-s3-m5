import { create } from 'zustand'

type EvalResponse = {
  ok: boolean
  output?: string
  error?: string | null
  timed_out?: boolean
}

type WsHistoryItem = {
  id: number
  rx_ms: number
  raw: string
  parsed: any | null
  parse_error: string | null
}

type EncoderMsg = {
  type: 'encoder'
  seq: number
  ts_ms: number
  pos: number
  delta: number
}

type EncoderClickMsg = {
  type: 'encoder_click'
  seq: number
  ts_ms: number
  kind: number // 0=single, 1=double, 2=long
}

type Store = {
  code: string
  running: boolean
  last: EvalResponse | null
  setCode: (code: string) => void
  run: () => Promise<void>
  wsConnected: boolean
  encoder: EncoderMsg | null
  lastClick: EncoderClickMsg | null
  wsHistory: WsHistoryItem[]
  clearWsHistory: () => void
  connectWs: () => void
}

let ws: WebSocket | null = null
let reconnectTimer: number | null = null
let reconnectBackoffMs = 250
let wsHistoryNextId = 1

const WS_HISTORY_MAX = 200

export const useStore = create<Store>((set, get) => ({
  code: '1+1',
  running: false,
  last: null,
  wsConnected: false,
  encoder: null,
  lastClick: null,
  wsHistory: [],
  clearWsHistory: () => set({ wsHistory: [] }),
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
      const raw = typeof ev.data === 'string' ? ev.data : ''
      let parsed: any | null = null
      let parse_error: string | null = null
      if (raw) {
        try {
          parsed = JSON.parse(raw)
        } catch (e: any) {
          parse_error = String(e)
        }
      }

      const id = wsHistoryNextId++
      const item: WsHistoryItem = { id, rx_ms: Date.now(), raw, parsed, parse_error }
      set((s) => {
        const next = s.wsHistory.length >= WS_HISTORY_MAX ? s.wsHistory.slice(s.wsHistory.length - WS_HISTORY_MAX + 1) : s.wsHistory
        return { wsHistory: [...next, item] }
      })

      if (parsed && typeof parsed === 'object') {
        const msg = parsed as EncoderMsg | EncoderClickMsg
        if (msg.type === 'encoder') set({ encoder: msg })
        if (msg.type === 'encoder_click') set({ lastClick: msg })
      }
    }
  },
}))
