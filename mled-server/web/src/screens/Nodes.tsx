import { useCallback } from 'preact/hooks';
import { useAppStore } from '../store';
import { apiClient, ApiError } from '../lib/apiClient';
import {
  useNodes,
  useSortedNodeIds,
  useOnlineNodes,
  usePresets,
  useSelectedCount,
  useIsNodeSelected,
} from '../lib/useStableSelector';
import type { Preset, PatternConfig } from '../types';

function formatRssi(rssi: number): string {
  if (rssi === 0) return '---';
  return `${rssi}dBm`;
}

function formatUptime(ms: number): string {
  if (ms === 0) return '---';
  const seconds = Math.floor(ms / 1000);
  const minutes = Math.floor(seconds / 60);
  const hours = Math.floor(minutes / 60);
  if (hours > 0) {
    return `${hours}h ${minutes % 60}m`;
  }
  return `${minutes}m`;
}

function getPatternLabel(config: PatternConfig | null): string {
  if (!config) return '---';
  switch (config.type) {
    case 'rainbow':
      return '🎨 Rainbow';
    case 'chase':
      return '🏃 Chase';
    case 'breathing':
      return '💨 Breathing';
    case 'sparkle':
      return '✨ Sparkle';
    case 'off':
      return '⬛ Off';
    default:
      return config.type;
  }
}

function NodeRow({ nodeId }: { nodeId: string }) {
  const node = useAppStore((s) => s.nodes.get(nodeId));
  const isSelected = useIsNodeSelected(nodeId);
  const toggleSelection = useAppStore((s) => s.toggleNodeSelection);

  if (!node) return null;

  const statusClass = node.status;
  const isOffline = node.status === 'offline';

  return (
    <li
      class={`node-row ${isSelected ? 'selected' : ''}`}
      onClick={() => toggleSelection(nodeId)}
    >
      <input
        type="checkbox"
        class="form-check-input"
        checked={isSelected}
        onClick={(e) => {
          e.stopPropagation();
          toggleSelection(nodeId);
        }}
        onChange={(e) => e.stopPropagation()}
        disabled={isOffline}
      />
      <span class={`status-dot ${statusClass}`} />
      <span class="flex-grow-1 fw-medium">
        {node.name || node.node_id}
      </span>
      <span class="text-secondary" style={{ minWidth: '70px' }}>
        {formatRssi(node.rssi)}
      </span>
      <span class="text-secondary" style={{ minWidth: '120px' }}>
        {isOffline ? (
          <span class="text-danger">
            offline ({formatUptime(Date.now() - node.last_seen)})
          </span>
        ) : (
          getPatternLabel(node.current_pattern)
        )}
      </span>
      {node.status === 'weak' && (
        <span class="badge bg-warning">weak</span>
      )}
    </li>
  );
}

function PresetButton({ preset, onApply }: { preset: Preset; onApply: (preset: Preset) => void }) {
  const handleClick = useCallback(() => {
    onApply(preset);
  }, [preset, onApply]);

  return (
    <button class="preset-btn" onClick={handleClick} type="button">
      <span>{preset.icon}</span>
      <span>{preset.name}</span>
    </button>
  );
}

export function NodesScreen() {
  // Use stable selector hooks
  const nodes = useNodes();
  const sortedNodeIds = useSortedNodeIds();
  const onlineNodes = useOnlineNodes();
  const presets = usePresets();
  const selectedCount = useSelectedCount();
  
  // Direct store access for primitives and actions (no memo needed)
  const selectedNodeIds = useAppStore((s) => s.selectedNodeIds);
  const globalBrightness = useAppStore((s) => s.globalBrightness);
  const setGlobalBrightness = useAppStore((s) => s.setGlobalBrightness);
  const selectAll = useAppStore((s) => s.selectAll);
  const selectNone = useAppStore((s) => s.selectNone);
  const addLogEntry = useAppStore((s) => s.addLogEntry);

  const offlineCount = nodes.length - onlineNodes.length;

  const handleApplyPreset = useCallback(async (preset: Preset) => {
    if (selectedNodeIds.size === 0) {
      addLogEntry('No nodes selected');
      return;
    }

    const nodeIds = Array.from(selectedNodeIds);
    try {
      addLogEntry(`Applying ${preset.name} to ${nodeIds.length} node(s)...`);
      const result = await apiClient.applyPattern({
        node_ids: nodeIds,
        config: {
          ...preset.config,
          brightness: globalBrightness,
        },
      });
      addLogEntry(`Applied to ${result.sent_to.length} node(s)`);
      if (result.failed.length > 0) {
        addLogEntry(`Failed for ${result.failed.length} node(s)`);
      }
    } catch (err) {
      if (err instanceof ApiError && err.status === 400) {
        addLogEntry(`Apply rejected (400): ${err.message}`);
        return;
      }
      addLogEntry(`Error: ${err instanceof Error ? err.message : 'Unknown error'}`);
    }
  }, [selectedNodeIds, globalBrightness, addLogEntry]);

  const handleApplySelected = useCallback(async () => {
    if (selectedNodeIds.size === 0) {
      addLogEntry('No nodes selected');
      return;
    }

    // Apply with the first preset (or rainbow as fallback)
    const defaultPreset = presets.find((p) => p.id === 'preset-rainbow') || presets[0];
    if (defaultPreset) {
      await handleApplyPreset(defaultPreset);
    }
  }, [selectedNodeIds, presets, handleApplyPreset, addLogEntry]);

  const handleToggleSelectAll = useCallback(() => {
    if (selectedCount === onlineNodes.length) {
      selectNone();
    } else {
      selectAll();
    }
  }, [selectedCount, onlineNodes.length, selectAll, selectNone]);

  const handleBrightnessChange = useCallback((e: Event) => {
    setGlobalBrightness(Number((e.target as HTMLInputElement).value));
  }, [setGlobalBrightness]);

  return (
    <div class="container-fluid px-0">
      {/* Nodes section */}
      <div class="card mb-4">
        <div class="card-header d-flex justify-content-between align-items-center">
          <span>Nodes</span>
          <button
            class="btn btn-sm btn-outline-primary"
            onClick={handleToggleSelectAll}
            type="button"
          >
            {selectedCount === onlineNodes.length && onlineNodes.length > 0 ? 'Deselect All' : 'Select All'}
          </button>
        </div>
        <ul class="node-list">
          {sortedNodeIds.length === 0 ? (
            <li class="node-row text-secondary">
              No nodes discovered yet. Make sure the backend is running.
            </li>
          ) : (
            sortedNodeIds.map((nodeId) => (
              <NodeRow key={nodeId} nodeId={nodeId} />
            ))
          )}
        </ul>
      </div>

      {/* Summary stats */}
      <div class="stats-summary mb-4">
        <span>
          <span class="status-dot online me-1" style={{ width: '8px', height: '8px' }} />
          {onlineNodes.length} online
        </span>
        <span>·</span>
        <span>
          <span class="status-dot offline me-1" style={{ width: '8px', height: '8px' }} />
          {offlineCount} offline
        </span>
        <span>·</span>
        <span class="text-primary">{selectedCount} selected</span>
      </div>

      {/* Quick apply section */}
      <div class="card mb-4">
        <div class="card-header">Quick Apply</div>
        <div class="card-body">
          <div class="d-flex flex-wrap gap-2 mb-3">
            {presets.slice(0, 5).map((preset) => (
              <PresetButton
                key={preset.id}
                preset={preset}
                onApply={handleApplyPreset}
              />
            ))}
          </div>
        </div>
      </div>

      {/* Brightness control */}
      <div class="brightness-control mb-4">
        <label class="form-label mb-0 text-secondary" style={{ minWidth: '80px' }}>
          Brightness:
        </label>
        <input
          type="range"
          class="form-range flex-grow-1"
          min="0"
          max="100"
          value={globalBrightness}
          onInput={handleBrightnessChange}
        />
        <span class="brightness-value">{globalBrightness}%</span>
        <button
          class="btn btn-primary"
          onClick={handleApplySelected}
          disabled={selectedCount === 0}
          type="button"
        >
          Apply to Selected
        </button>
      </div>

      {/* Spacer for bottom nav */}
      <div style={{ height: '80px' }} />
    </div>
  );
}
