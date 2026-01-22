import type {
  Node,
  Preset,
  Settings,
  ControllerStatus,
  ApplyRequest,
  ApplyResponse,
  NetworkInterface,
} from './types';
import { useAppStore } from './store';

const API_BASE = '/api';

// Helper for JSON requests
async function fetchJson<T>(
  path: string,
  options?: RequestInit
): Promise<T> {
  const res = await fetch(`${API_BASE}${path}`, {
    ...options,
    headers: {
      'Content-Type': 'application/json',
      ...options?.headers,
    },
  });

  if (!res.ok) {
    const text = await res.text();
    throw new Error(`API Error: ${res.status} ${text}`);
  }

  return res.json();
}

// === Nodes API ===

export async function fetchNodes(): Promise<Node[]> {
  // Backend returns array directly, not wrapped in { nodes: [...] }
  return fetchJson<Node[]>('/nodes');
}

export async function discoverNodes(timeoutMs: number = 500): Promise<Node[]> {
  const data = await fetchJson<{ nodes: Node[] }>('/nodes/discover', {
    method: 'POST',
    body: JSON.stringify({ timeoutMs }),
  });
  return data.nodes;
}

export async function pingNodes(): Promise<void> {
  await fetchJson<{ ok: boolean }>('/nodes/ping', {
    method: 'POST',
  });
}

// === Apply / Pattern API ===

export async function applyPattern(request: ApplyRequest): Promise<ApplyResponse> {
  return fetchJson<ApplyResponse>('/apply', {
    method: 'POST',
    body: JSON.stringify(request),
  });
}

// === Presets API ===

export async function fetchPresets(): Promise<Preset[]> {
  const data = await fetchJson<Preset[] | null>('/presets');
  return data || []; // Backend returns null if no presets saved
}

export async function createPreset(preset: Preset): Promise<Preset> {
  return fetchJson<Preset>('/presets', {
    method: 'POST',
    body: JSON.stringify(preset),
  });
}

export async function updatePreset(preset: Preset): Promise<Preset> {
  return fetchJson<Preset>(`/presets/${preset.id}`, {
    method: 'PUT',
    body: JSON.stringify(preset),
  });
}

export async function deletePreset(id: string): Promise<void> {
  await fetchJson<{ ok: boolean }>(`/presets/${id}`, {
    method: 'DELETE',
  });
}

// === Status API ===

interface BackendStatus {
  epoch_id: number;
  running: boolean;
  show_ms: number;
}

export async function fetchStatus(): Promise<Partial<ControllerStatus>> {
  const data = await fetchJson<BackendStatus>('/status');
  return {
    epochId: data.epoch_id,
    showMs: data.show_ms,
    connected: data.running,
  };
}

// === Settings API ===

export async function fetchSettings(): Promise<Settings> {
  return fetchJson<Settings>('/settings');
}

export async function updateSettings(settings: Settings): Promise<Settings> {
  return fetchJson<Settings>('/settings', {
    method: 'PUT',
    body: JSON.stringify(settings),
  });
}

export async function fetchNetworkInterfaces(): Promise<NetworkInterface[]> {
  return fetchJson<NetworkInterface[]>('/net/interfaces');
}

// === SSE Events ===

let eventSource: EventSource | null = null;
let lastErrorTime = 0;
let sseEnabled = true;

export function connectSSE(): void {
  if (!sseEnabled) return;
  
  if (eventSource) {
    eventSource.close();
  }

  const store = useAppStore.getState();

  eventSource = new EventSource(`${API_BASE}/events`);

  eventSource.onopen = () => {
    store.setConnected(true);
    store.setLastError(null);
    store.addLogEntry('Connected to server');
  };

  eventSource.onerror = () => {
    // Debounce error handling to prevent rapid state updates
    const now = Date.now();
    if (now - lastErrorTime < 5000) {
      // If we've had an error in the last 5 seconds, don't update state again
      return;
    }
    lastErrorTime = now;

    store.setConnected(false);
    store.setLastError('Connection lost. Reconnecting...');
    store.addLogEntry('Connection lost');

    // Close and disable SSE after repeated failures (backend not running)
    if (eventSource) {
      eventSource.close();
      eventSource = null;
    }
    sseEnabled = false;
    
    // Re-enable SSE after 10 seconds
    setTimeout(() => {
      sseEnabled = true;
    }, 10000);
  };

  // Node update event
  eventSource.addEventListener('node', (event: MessageEvent) => {
    try {
      const node: Node = JSON.parse(event.data);
      store.updateNode(node);
    } catch (e) {
      console.error('Failed to parse node event:', e);
    }
  });

  // Node offline event
  eventSource.addEventListener('node.offline', (event: MessageEvent) => {
    try {
      const { node_id } = JSON.parse(event.data);
      const existingNode = store.nodes.get(node_id);
      if (existingNode) {
        store.updateNode({ ...existingNode, status: 'offline' });
        store.addLogEntry(`Node ${existingNode.name || node_id} went offline`);
      }
    } catch (e) {
      console.error('Failed to parse offline event:', e);
    }
  });

  // Ack event
  eventSource.addEventListener('ack', (event: MessageEvent) => {
    try {
      const { node_id, success } = JSON.parse(event.data);
      const node = store.nodes.get(node_id);
      const name = node?.name || node_id;
      store.addLogEntry(`ACK from ${name}: ${success ? 'OK' : 'FAILED'}`);
    } catch (e) {
      console.error('Failed to parse ack event:', e);
    }
  });

  // Log event
  eventSource.addEventListener('log', (event: MessageEvent) => {
    try {
      const { message } = JSON.parse(event.data);
      store.addLogEntry(message);
    } catch (e) {
      console.error('Failed to parse log event:', e);
    }
  });
}

export function disconnectSSE(): void {
  if (eventSource) {
    eventSource.close();
    eventSource = null;
    useAppStore.getState().setConnected(false);
  }
}

// === Initial data loading ===

export async function loadInitialData(): Promise<void> {
  const store = useAppStore.getState();

  try {
    // Load all data in parallel
    const [nodes, presets, settings, status] = await Promise.all([
      fetchNodes().catch(() => [] as Node[]),
      fetchPresets().catch(() => null),
      fetchSettings().catch(() => null),
      fetchStatus().catch(() => null),
    ]);

    store.setNodes(nodes);

    if (presets) {
      store.setPresets(presets);
    }

    if (settings) {
      store.setSettings(settings);
    }

    if (status) {
      store.setStatus(status);
    }

    store.setConnected(true);
    store.setLastError(null);
    
    // Re-enable SSE if it was disabled
    if (!sseEnabled) {
      sseEnabled = true;
      connectSSE();
    }
  } catch (error) {
    store.setConnected(false);
    store.setLastError(error instanceof Error ? error.message : 'Failed to load data');
  }
}
