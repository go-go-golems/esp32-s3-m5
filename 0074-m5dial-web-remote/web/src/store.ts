import { create } from 'zustand'

export type DeviceState = {
  device_id: string
  connected: boolean
  remote_addr: string
  last_type: string
  last_button?: string
  last_seq: number
  position: number
  last_delta: number
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

type WsFrame = {
  id: number
  rxMs: number
  raw: string
  parseError: string | null
}

type Store = {
  wsConnected: boolean
  wsFrames: WsFrame[]
  devices: DeviceState[]
  history: HistoryEntry[]
  selectedDeviceId: string
  connectWs: () => void
  selectDevice: (id: string) => void
  clearFrames: () => void
}

let socket: WebSocket | null = null
let reconnectTimer: number | null = null
let reconnectBackoffMs = 300
let nextFrameID = 1
const maxFrames = 200

function ensureSelectedDevice(devices: DeviceState[], selectedDeviceId: string) {
  if (selectedDeviceId && devices.some((device) => device.device_id === selectedDeviceId)) {
    return selectedDeviceId
  }
  return devices[0]?.device_id ?? ''
}

export const useStore = create<Store>((set, get) => ({
  wsConnected: false,
  wsFrames: [],
  devices: [],
  history: [],
  selectedDeviceId: '',
  connectWs: () => {
    if (socket && (socket.readyState === WebSocket.OPEN || socket.readyState === WebSocket.CONNECTING)) {
      return
    }
    if (reconnectTimer !== null) {
      window.clearTimeout(reconnectTimer)
      reconnectTimer = null
    }

    const proto = window.location.protocol === 'https:' ? 'wss' : 'ws'
    socket = new WebSocket(`${proto}://${window.location.host}/ws/browser`)

    socket.onopen = () => {
      reconnectBackoffMs = 300
      set({ wsConnected: true })
    }

    socket.onclose = () => {
      set({ wsConnected: false })
      if (reconnectTimer === null) {
        reconnectTimer = window.setTimeout(() => {
          reconnectTimer = null
          get().connectWs()
        }, reconnectBackoffMs)
        reconnectBackoffMs = Math.min(5000, reconnectBackoffMs * 2)
      }
    }

    socket.onerror = () => {
      // Close handler drives reconnect scheduling.
    }

    socket.onmessage = (event) => {
      const raw = typeof event.data === 'string' ? event.data : ''
      let parsed: unknown = null
      let parseError: string | null = null
      try {
        parsed = JSON.parse(raw)
      } catch (error) {
        parseError = String(error)
      }

      const frame: WsFrame = {
        id: nextFrameID++,
        rxMs: Date.now(),
        raw,
        parseError,
      }

      set((state) => {
        const wsFrames =
          state.wsFrames.length >= maxFrames
            ? [...state.wsFrames.slice(state.wsFrames.length - maxFrames + 1), frame]
            : [...state.wsFrames, frame]

        if (!parsed || typeof parsed !== 'object' || parseError) {
          return { wsFrames }
        }

        const msg = parsed as { type?: string }
        if (msg.type === 'server_snapshot' || msg.type === 'server_status') {
          const snapshot = parsed as ServerSnapshot
          return {
            wsFrames,
            devices: snapshot.devices,
            history: snapshot.history,
            selectedDeviceId: ensureSelectedDevice(snapshot.devices, state.selectedDeviceId),
          }
        }

        return { wsFrames }
      })
    }
  },
  selectDevice: (id) => set({ selectedDeviceId: id }),
  clearFrames: () => set({ wsFrames: [] }),
}))
