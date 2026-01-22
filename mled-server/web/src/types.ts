// Node status types
export type NodeStatus = 'online' | 'weak' | 'offline';

// Pattern types supported by MLED/1 protocol (matches protocol.h)
// 0=OFF, 1=RAINBOW, 2=CHASE, 3=BREATHING, 4=SPARKLE
export type PatternType = 'off' | 'rainbow' | 'chase' | 'breathing' | 'sparkle';

// Pattern configuration - matches mled_pattern_config_t wire format
export interface PatternConfig {
  type: PatternType;
  brightness: number; // 0-100 (brightness_pct)
  params: PatternParams;
}

// Pattern-specific params (maps to data[12] in wire format)
export type PatternParams = 
  | RainbowParams
  | ChaseParams
  | BreathingParams
  | SparkleParams
  | OffParams;

// RAINBOW (type 1): rotating hue cycle
export interface RainbowParams {
  speed: number;        // 0-20, rotations/min
  saturation: number;   // 0-100
  spread_x10: number;   // 1-50 (spread = value/10)
}

// CHASE (type 2): moving segments
export interface ChaseParams {
  speed: number;        // 0-255, LEDs/sec
  tail_len: number;     // 1-255
  gap_len: number;      // 0-255
  trains: number;       // 1-255, number of chase segments
  fg_color: string;     // hex color #RRGGBB
  bg_color: string;     // hex color #RRGGBB
  direction: 'forward' | 'reverse' | 'bounce';  // 0, 1, 2
  fade_tail: boolean;   // 0 or 1
}

// BREATHING (type 3): pulsing brightness
export interface BreathingParams {
  speed: number;        // 0-20, breaths/min
  color: string;        // hex color #RRGGBB
  min_bri: number;      // 0-255, min brightness
  max_bri: number;      // 0-255, max brightness
  curve: 'sine' | 'linear' | 'ease';  // 0, 1, 2
}

// SPARKLE (type 4): random twinkling
export interface SparkleParams {
  speed: number;        // 0-20, spawn rate control
  color: string;        // hex color #RRGGBB
  density_pct: number;  // 0-100
  fade_speed: number;   // 1-255
  color_mode: 'fixed' | 'random' | 'rainbow';  // 0, 1, 2
  bg_color: string;     // hex color #RRGGBB
}

// OFF (type 0): all LEDs off
export interface OffParams {
  // No params needed
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
