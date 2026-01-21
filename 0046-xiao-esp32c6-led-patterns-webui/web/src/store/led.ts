import { create } from 'zustand'

export type LedStatus = {
  ok: boolean
  running: boolean
  paused: boolean
  frame_ms: number
  pattern: {
    type: 'off' | 'rainbow' | 'chase' | 'breathing' | 'sparkle' | string
    global_brightness_pct: number
    rainbow?: {
      speed: number
      sat: number
      spread: number
    }
    chase?: {
      speed: number
      tail: number
      gap: number
      trains: number
      fg: string
      bg: string
      dir: 'forward' | 'reverse' | 'bounce' | string
      fade: boolean
    }
    breathing?: {
      speed: number
      color: string
      min: number
      max: number
      curve: 'sine' | 'linear' | 'ease' | string
    }
    sparkle?: {
      speed: number
      color: string
      density: number
      fade: number
      mode: 'fixed' | 'random' | 'rainbow' | string
      bg: string
    }
  }
}

type LedStore = {
  status: LedStatus | null
  err: string | null
  refresh: () => Promise<void>
  setPattern: (type: string) => Promise<void>
  setBrightness: (brightness_pct: number) => Promise<void>
  setFrame: (frame_ms: number) => Promise<void>
  setRainbow: (cfg: Partial<NonNullable<LedStatus['pattern']['rainbow']>>) => Promise<void>
  setChase: (cfg: Partial<NonNullable<LedStatus['pattern']['chase']>>) => Promise<void>
  setBreathing: (cfg: Partial<NonNullable<LedStatus['pattern']['breathing']>>) => Promise<void>
  setSparkle: (cfg: Partial<NonNullable<LedStatus['pattern']['sparkle']>>) => Promise<void>
  pause: () => Promise<void>
  resume: () => Promise<void>
  clear: () => Promise<void>
}

async function postJson(path: string, body?: unknown): Promise<void> {
  const res = await fetch(path, {
    method: 'POST',
    headers: body ? { 'Content-Type': 'application/json' } : undefined,
    body: body ? JSON.stringify(body) : undefined,
  })
  if (!res.ok) {
    const t = await res.text().catch(() => '')
    throw new Error(`${res.status} ${res.statusText}${t ? `: ${t}` : ''}`)
  }
}

export const useLedStore = create<LedStore>((set, get) => ({
  status: null,
  err: null,
  refresh: async () => {
    try {
      const res = await fetch('/api/led/status')
      if (!res.ok) throw new Error(`${res.status} ${res.statusText}`)
      const j = (await res.json()) as LedStatus
      set({ status: j, err: null })
    } catch (e) {
      set({ err: String(e) })
    }
  },
  setPattern: async (type) => {
    await postJson('/api/led/pattern', { type })
    await get().refresh()
  },
  setBrightness: async (brightness_pct) => {
    await postJson('/api/led/brightness', { brightness_pct })
    await get().refresh()
  },
  setFrame: async (frame_ms) => {
    await postJson('/api/led/frame', { frame_ms })
    await get().refresh()
  },
  setRainbow: async (cfg) => {
    await postJson('/api/led/rainbow', cfg)
    await get().refresh()
  },
  setChase: async (cfg) => {
    await postJson('/api/led/chase', cfg)
    await get().refresh()
  },
  setBreathing: async (cfg) => {
    await postJson('/api/led/breathing', cfg)
    await get().refresh()
  },
  setSparkle: async (cfg) => {
    await postJson('/api/led/sparkle', cfg)
    await get().refresh()
  },
  pause: async () => {
    await postJson('/api/led/pause')
    await get().refresh()
  },
  resume: async () => {
    await postJson('/api/led/resume')
    await get().refresh()
  },
  clear: async () => {
    await postJson('/api/led/clear')
    await get().refresh()
  },
}))
