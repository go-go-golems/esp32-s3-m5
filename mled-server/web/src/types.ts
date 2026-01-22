// Node status types
export type NodeStatus = 'online' | 'weak' | 'offline';

// Pattern types supported by MLED nodes
export type PatternType = 'solid' | 'rainbow' | 'gradient' | 'pulse' | 'off';

// Pattern configuration
export interface PatternConfig {
  type: PatternType;
  brightness: number; // 0-100
  params: Record<string, number | string>;
}

// MLED Node
export interface Node {
  node_id: string;        // hex, e.g. "4A7F2C01"
  name: string;           // from PONG or user-assigned
  ip: string;
  port: number;
  rssi: number;           // dBm, from PONG
  uptime_ms: number;      // from PONG
  last_seen: number;      // unix timestamp ms
  current_pattern: PatternConfig | null;
  status: NodeStatus;
}

// Preset (saved pattern configuration)
export interface Preset {
  id: string;             // uuid
  name: string;
  icon: string;           // emoji
  config: PatternConfig;
}

// Settings
export interface Settings {
  bind_ip: string;
  multicast_group: string;
  multicast_port: number;
  discovery_interval_ms: number;
  offline_threshold_s: number;
}

// Controller status
export interface ControllerStatus {
  epochId: number;
  showMs: number;
  beaconIntervalMs: number;
  version: string;
  uptimeMs: number;
  connected: boolean;
  bindIp: string;
  multicastGroup: string;
  multicastPort: number;
}

// Apply request
export interface ApplyRequest {
  node_ids: string[] | 'all';
  config: PatternConfig;
}

// Apply response
export interface ApplyResponse {
  sent_to: string[];
  failed: string[];
}

// SSE Event types
export interface NodeUpdateEvent {
  type: 'node.update';
  data: Node;
}

export interface NodeOfflineEvent {
  type: 'node.offline';
  data: { node_id: string };
}

export interface ApplyAckEvent {
  type: 'apply.ack';
  data: { node_id: string; success: boolean };
}

export interface ErrorEvent {
  type: 'error';
  data: { message: string };
}

export type SSEEvent = NodeUpdateEvent | NodeOfflineEvent | ApplyAckEvent | ErrorEvent;

// Network interface for settings
export interface NetworkInterface {
  name: string;
  ip: string;
}
