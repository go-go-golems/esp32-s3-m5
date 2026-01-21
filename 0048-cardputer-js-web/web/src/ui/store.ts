import { create } from 'zustand'

type EvalResponse = {
  ok: boolean
  output?: string
  error?: string | null
  timed_out?: boolean
}

type Store = {
  code: string
  running: boolean
  last: EvalResponse | null
  setCode: (code: string) => void
  run: () => Promise<void>
}

export const useStore = create<Store>((set, get) => ({
  code: '1+1',
  running: false,
  last: null,
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
}))

