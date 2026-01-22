/**
 * Pattern 3: API Client with Response Transformers
 * 
 * Problem: Backend and frontend have different response formats.
 * When they drift, we get runtime errors.
 * 
 * Solution: Create explicit transformer functions that:
 * - Define the exact backend response shape
 * - Transform to the frontend's expected shape
 * - Provide sensible defaults for missing fields
 * - Make the contract explicit and type-safe
 */

import type {
  Node,
  Preset,
  Settings,
  ControllerStatus,
  ApplyRequest,
  ApplyResponse,
  NetworkInterface,
} from '../types';

const API_BASE = '/api';

// ============================================================================
// Backend Response Types (what the server actually returns)
// ============================================================================

interface BackendNode {
  node_id: string;
  name: string;
  ip: string;
  port: number;
  rssi: number;
  uptime_ms: number;
  last_seen: number;
  current_pattern: {
    type: string;
    brightness: number;
    params: Record<string, unknown>;
  } | null;
  status: string;
}

interface BackendStatus {
  epoch_id: number;
  running: boolean;
  show_ms: number;
}

interface BackendSettings {
  bind_ip: string;
  multicast_group: string;
  multicast_port: number;
  discovery_interval_ms: number;
  offline_threshold_s: number;
}

interface BackendApplyResponse {
  sent_to?: string[];
  failed?: string[];
  success?: boolean;
}

// ============================================================================
// Transformers (backend → frontend)
// ============================================================================

function transformNode(backend: BackendNode): Node {
  return {
    node_id: backend.node_id,
    name: backend.name || backend.node_id,
    ip: backend.ip,
    port: backend.port,
    rssi: backend.rssi,
    uptime_ms: backend.uptime_ms,
    last_seen: backend.last_seen,
    current_pattern: backend.current_pattern ? {
      type: backend.current_pattern.type as Node['current_pattern']['type'],
      brightness: backend.current_pattern.brightness,
      params: backend.current_pattern.params as Record<string, number | string>,
    } : null,
    status: (backend.status as Node['status']) || 'offline',
  };
}

function transformStatus(backend: BackendStatus): Partial<ControllerStatus> {
  return {
    epochId: backend.epoch_id,
    showMs: backend.show_ms,
    connected: backend.running,
  };
}

function transformSettings(backend: BackendSettings): Settings {
  return {
    bind_ip: backend.bind_ip || 'auto',
    multicast_group: backend.multicast_group || '239.255.32.6',
    multicast_port: backend.multicast_port || 4626,
    discovery_interval_ms: backend.discovery_interval_ms || 1000,
    offline_threshold_s: backend.offline_threshold_s || 30,
  };
}

function transformApplyResponse(backend: BackendApplyResponse): ApplyResponse {
  return {
    sent_to: backend.sent_to || [],
    failed: backend.failed || [],
  };
}

// ============================================================================
// HTTP Helpers
// ============================================================================

class ApiError extends Error {
  constructor(public status: number, message: string) {
    super(message);
    this.name = 'ApiError';
  }
}

async function fetchJson<T>(path: string, options?: RequestInit): Promise<T> {
  const res = await fetch(`${API_BASE}${path}`, {
    ...options,
    headers: {
      'Content-Type': 'application/json',
      ...options?.headers,
    },
  });

  if (!res.ok) {
    const text = await res.text();
    throw new ApiError(res.status, `API Error: ${res.status} ${text}`);
  }

  return res.json();
}

// ============================================================================
// API Client
// ============================================================================

export const apiClient = {
  // === Nodes ===
  
  async fetchNodes(): Promise<Node[]> {
    const data = await fetchJson<BackendNode[] | null>('/nodes');
    return (data || []).map(transformNode);
  },

  async discoverNodes(timeoutMs: number = 500): Promise<Node[]> {
    const data = await fetchJson<BackendNode[] | null>('/nodes/discover', {
      method: 'POST',
      body: JSON.stringify({ timeoutMs }),
    });
    return (data || []).map(transformNode);
  },

  async pingNodes(): Promise<void> {
    await fetchJson<unknown>('/nodes/ping', { method: 'POST' });
  },

  // === Apply ===
  
  async applyPattern(request: ApplyRequest): Promise<ApplyResponse> {
    const data = await fetchJson<BackendApplyResponse>('/apply', {
      method: 'POST',
      body: JSON.stringify(request),
    });
    return transformApplyResponse(data);
  },

  // === Presets ===
  
  async fetchPresets(): Promise<Preset[]> {
    const data = await fetchJson<Preset[] | null>('/presets');
    return data || [];
  },

  async createPreset(preset: Preset): Promise<Preset> {
    return fetchJson<Preset>('/presets', {
      method: 'POST',
      body: JSON.stringify(preset),
    });
  },

  async updatePreset(preset: Preset): Promise<Preset> {
    return fetchJson<Preset>(`/presets/${preset.id}`, {
      method: 'PUT',
      body: JSON.stringify(preset),
    });
  },

  async deletePreset(id: string): Promise<void> {
    await fetchJson<unknown>(`/presets/${id}`, { method: 'DELETE' });
  },

  // === Status ===
  
  async fetchStatus(): Promise<Partial<ControllerStatus>> {
    const data = await fetchJson<BackendStatus>('/status');
    return transformStatus(data);
  },

  // === Settings ===
  
  async fetchSettings(): Promise<Settings> {
    const data = await fetchJson<BackendSettings>('/settings');
    return transformSettings(data);
  },

  async updateSettings(settings: Settings): Promise<Settings> {
    const data = await fetchJson<BackendSettings>('/settings', {
      method: 'PUT',
      body: JSON.stringify(settings),
    });
    return transformSettings(data);
  },

  async fetchNetworkInterfaces(): Promise<NetworkInterface[]> {
    const data = await fetchJson<NetworkInterface[] | null>('/net/interfaces');
    return data || [];
  },
};

export { ApiError };
