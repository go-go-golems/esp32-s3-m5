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
  }
}

type WsFrame = {
  id: number
  rxMs: number
  raw: string
  parseError: string | null
}

export type CommandFeedback = {
  requestId: number
  deviceId: string
  command: string
  status: string
  detail: string
  atMs: number
}

type CommandPayload = {
  text?: string
  value?: number
}

type Store = {
  wsConnected: boolean
  wsFrames: WsFrame[]
  devices: DeviceState[]
  history: HistoryEntry[]
  selectedDeviceId: string
  lastCommandFeedback: CommandFeedback | null
  connectWs: () => void
  selectDevice: (id: string) => void
  clearFrames: () => void
  sendUiCommand: (deviceId: string, command: string, payload?: CommandPayload) => void
}

let socket: WebSocket | null = null
let reconnectTimer: number | null = null
let reconnectBackoffMs = 300
let nextFrameID = 1
let nextRequestID = 1
const maxFrames = 200

function ensureSelectedDevice(devices: DeviceState[], selectedDeviceId: string) {
  if (selectedDeviceId && devices.some((device) => device.device_id === selectedDeviceId)) {
    return selectedDeviceId
  }
  return devices[0]?.device_id ?? ''
}

function setCommandFeedback(
  requestId: number,
  deviceId: string,
  command: string,
  status: string,
  detail: string,
): Pick<Store, 'lastCommandFeedback'> {
  return {
    lastCommandFeedback: {
      requestId,
      deviceId,
      command,
      status,
      detail,
      atMs: Date.now(),
    },
  }
}

export const useStore = create<Store>((set, get) => ({
  wsConnected: false,
  wsFrames: [],
  devices: [],
  history: [],
  selectedDeviceId: '',
  lastCommandFeedback: null,
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

        if (msg.type === 'ui_command_result') {
          const result = parsed as UiCommandResultFrame
          return {
            wsFrames,
            ...setCommandFeedback(
              result.request_id,
              result.device_id,
              result.command,
              result.status,
              result.reason ?? 'server accepted command for delivery',
            ),
          }
        }

        if (msg.type === 'device_event') {
          const deviceEvent = parsed as DeviceEventFrame
          if (deviceEvent.payload?.type === 'ui_command_ack') {
            return {
              wsFrames,
              ...setCommandFeedback(
                deviceEvent.payload.request_id ?? 0,
                deviceEvent.device_id,
                deviceEvent.payload.command ?? 'unknown',
                `ack:${deviceEvent.payload.status ?? 'unknown'}`,
                deviceEvent.payload.text ?? `position ${deviceEvent.payload.pos ?? 0}`,
              ),
            }
          }
        }

        return { wsFrames }
      })
    }
  },
  selectDevice: (id) => set({ selectedDeviceId: id }),
  clearFrames: () => set({ wsFrames: [] }),
  sendUiCommand: (deviceId, command, payload = {}) => {
    const requestId = nextRequestID++

    if (!deviceId) {
      set(setCommandFeedback(requestId, '', command, 'rejected', 'select a device first'))
      return
    }

    if (!socket || socket.readyState !== WebSocket.OPEN) {
      set(setCommandFeedback(requestId, deviceId, command, 'rejected', 'browser websocket is not connected'))
      return
    }

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
      set(setCommandFeedback(requestId, deviceId, command, 'sent', 'sent to server websocket'))
    } catch (error) {
      set(setCommandFeedback(requestId, deviceId, command, 'rejected', String(error)))
    }
  },
}))
