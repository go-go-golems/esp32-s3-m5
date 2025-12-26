export type Status = {
  ok: boolean
  ssid?: string
  ap_ip?: string
}

export type GraphicsEntry = {
  name: string
}

export async function apiStatus(): Promise<Status> {
  const res = await fetch('/api/status')
  return await res.json()
}

export async function apiGraphicsList(): Promise<GraphicsEntry[]> {
  const res = await fetch('/api/graphics')
  return await res.json()
}

export async function apiGraphicsUpload(name: string, file: File): Promise<void> {
  const res = await fetch(`/api/graphics/${name}`, {
    method: 'PUT',
    body: file,
  })
  if (!res.ok) {
    const txt = await res.text().catch(() => '')
    throw new Error(txt || `upload failed (${res.status})`)
  }
}


