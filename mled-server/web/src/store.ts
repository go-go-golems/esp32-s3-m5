import { create } from 'zustand';
import type { Node, Preset, Settings, ControllerStatus, PatternConfig } from './types';

// Default presets
export const DEFAULT_PRESETS: Preset[] = [
  {
    id: 'preset-rainbow',
    name: 'Rainbow',
    icon: '🌈',
    config: {
      type: 'rainbow',
      brightness: 75,
      params: { speed: 50 },
    },
  },
  {
    id: 'preset-calm',
    name: 'Calm',
    icon: '💙',
    config: {
      type: 'solid',
      brightness: 60,
      params: { color: '#2244AA' },
    },
  },
  {
    id: 'preset-warm',
    name: 'Warm',
    icon: '🔥',
    config: {
      type: 'gradient',
      brightness: 70,
      params: { color_start: '#FF6B35', color_end: '#AA2200' },
    },
  },
  {
    id: 'preset-pulse',
    name: 'Pulse',
    icon: '⚡',
    config: {
      type: 'pulse',
      brightness: 80,
      params: { color: '#FFFFFF', speed: 60 },
    },
  },
  {
    id: 'preset-off',
    name: 'Off',
    icon: '⬛',
    config: {
      type: 'off',
      brightness: 0,
      params: {},
    },
  },
];

// Default settings
export const DEFAULT_SETTINGS: Settings = {
  bind_ip: 'auto',
  multicast_group: '239.255.0.1',
  multicast_port: 5000,
  discovery_interval_ms: 1000,
  offline_threshold_s: 30,
};

// Default controller status
const DEFAULT_STATUS: ControllerStatus = {
  epochId: 0,
  showMs: 0,
  beaconIntervalMs: 500,
  version: 'dev',
  uptimeMs: 0,
  connected: false,
  bindIp: '',
  multicastGroup: '239.255.0.1',
  multicastPort: 5000,
};

// Tab types
export type TabId = 'nodes' | 'patterns' | 'status';

export interface AppState {
  // Nodes
  nodes: Map<string, Node>;
  selectedNodeIds: Set<string>;

  // Presets
  presets: Preset[];
  editingPresetId: string | null;

  // UI
  currentTab: TabId;
  globalBrightness: number;
  showSettings: boolean;

  // Controller status
  status: ControllerStatus;
  settings: Settings;

  // Connection
  connected: boolean;
  lastError: string | null;

  // Activity log
  activityLog: string[];

  // Actions - Nodes
  setNodes: (nodes: Node[]) => void;
  updateNode: (node: Node) => void;
  removeNode: (nodeId: string) => void;
  selectNode: (id: string, selected: boolean) => void;
  selectAll: () => void;
  selectNone: () => void;
  toggleNodeSelection: (id: string) => void;

  // Actions - Presets
  setPresets: (presets: Preset[]) => void;
  addPreset: (preset: Preset) => void;
  updatePreset: (preset: Preset) => void;
  deletePreset: (id: string) => void;
  setEditingPresetId: (id: string | null) => void;

  // Actions - UI
  setCurrentTab: (tab: TabId) => void;
  setGlobalBrightness: (value: number) => void;
  setShowSettings: (show: boolean) => void;

  // Actions - Status
  setStatus: (status: Partial<ControllerStatus>) => void;
  setSettings: (settings: Settings) => void;

  // Actions - Connection
  setConnected: (connected: boolean) => void;
  setLastError: (error: string | null) => void;

  // Actions - Activity log
  addLogEntry: (entry: string) => void;
  clearLog: () => void;
}

export const useAppStore = create<AppState>((set, get) => ({
  // Initial state
  nodes: new Map(),
  selectedNodeIds: new Set(),
  presets: DEFAULT_PRESETS,
  editingPresetId: null,
  currentTab: 'nodes',
  globalBrightness: 70,
  showSettings: false,
  status: DEFAULT_STATUS,
  settings: DEFAULT_SETTINGS,
  connected: false,
  lastError: null,
  activityLog: [],

  // Node actions
  setNodes: (nodes) =>
    set(() => ({
      nodes: new Map((nodes || []).map((n) => [n.node_id, n])),
    })),

  updateNode: (node) =>
    set((state) => {
      const newNodes = new Map(state.nodes);
      newNodes.set(node.node_id, node);
      return { nodes: newNodes };
    }),

  removeNode: (nodeId) =>
    set((state) => {
      const newNodes = new Map(state.nodes);
      newNodes.delete(nodeId);
      const newSelected = new Set(state.selectedNodeIds);
      newSelected.delete(nodeId);
      return { nodes: newNodes, selectedNodeIds: newSelected };
    }),

  selectNode: (id, selected) =>
    set((state) => {
      const newSelected = new Set(state.selectedNodeIds);
      if (selected) {
        newSelected.add(id);
      } else {
        newSelected.delete(id);
      }
      return { selectedNodeIds: newSelected };
    }),

  selectAll: () =>
    set((state) => ({
      selectedNodeIds: new Set(
        Array.from(state.nodes.values())
          .filter((n) => n.status !== 'offline')
          .map((n) => n.node_id)
      ),
    })),

  selectNone: () => set({ selectedNodeIds: new Set() }),

  toggleNodeSelection: (id) =>
    set((state) => {
      const newSelected = new Set(state.selectedNodeIds);
      if (newSelected.has(id)) {
        newSelected.delete(id);
      } else {
        newSelected.add(id);
      }
      return { selectedNodeIds: newSelected };
    }),

  // Preset actions
  setPresets: (presets) => set({ presets }),

  addPreset: (preset) =>
    set((state) => ({
      presets: [...state.presets, preset],
    })),

  updatePreset: (preset) =>
    set((state) => ({
      presets: state.presets.map((p) => (p.id === preset.id ? preset : p)),
    })),

  deletePreset: (id) =>
    set((state) => ({
      presets: state.presets.filter((p) => p.id !== id),
      editingPresetId: state.editingPresetId === id ? null : state.editingPresetId,
    })),

  setEditingPresetId: (id) => set({ editingPresetId: id }),

  // UI actions
  setCurrentTab: (tab) => set({ currentTab: tab }),
  setGlobalBrightness: (value) => set({ globalBrightness: value }),
  setShowSettings: (show) => set({ showSettings: show }),

  // Status actions
  setStatus: (status) =>
    set((state) => ({
      status: { ...state.status, ...status },
    })),

  setSettings: (settings) => set({ settings }),

  // Connection actions
  setConnected: (connected) =>
    set((state) => ({
      connected,
      status: { ...state.status, connected },
    })),

  setLastError: (error) => set({ lastError: error }),

  // Activity log actions
  addLogEntry: (entry) =>
    set((state) => ({
      activityLog: [
        `[${new Date().toLocaleTimeString()}] ${entry}`,
        ...state.activityLog.slice(0, 99),
      ],
    })),

  clearLog: () => set({ activityLog: [] }),
}));

// Derived selectors
export const useOnlineNodes = () =>
  useAppStore((state) =>
    Array.from(state.nodes.values()).filter((n) => n.status !== 'offline')
  );

export const useOfflineNodes = () =>
  useAppStore((state) =>
    Array.from(state.nodes.values()).filter((n) => n.status === 'offline')
  );

export const useWeakNodes = () =>
  useAppStore((state) =>
    Array.from(state.nodes.values()).filter((n) => n.status === 'weak')
  );

export const useSelectedNodes = () =>
  useAppStore((state) =>
    Array.from(state.nodes.values()).filter((n) =>
      state.selectedNodeIds.has(n.node_id)
    )
  );

export const useProblematicNodes = () =>
  useAppStore((state) =>
    Array.from(state.nodes.values()).filter(
      (n) => n.status === 'offline' || n.status === 'weak'
    )
  );

export const useEditingPreset = () =>
  useAppStore((state) =>
    state.presets.find((p) => p.id === state.editingPresetId) || null
  );
