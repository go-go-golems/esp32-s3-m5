import { useAppStore, useSelectedNodes } from '../store';
import { applyPattern } from '../api';
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
    case 'solid':
      const color = config.params.color as string;
      return `🎨 Solid ${color?.toUpperCase() || ''}`;
    case 'gradient':
      return '🎨 Gradient';
    case 'pulse':
      return '⚡ Pulse';
    case 'off':
      return '⬛ Off';
    default:
      return config.type;
  }
}

function NodeRow({ nodeId }: { nodeId: string }) {
  const node = useAppStore((s) => s.nodes.get(nodeId));
  const isSelected = useAppStore((s) => s.selectedNodeIds.has(nodeId));
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
        onChange={(e) => {
          e.stopPropagation();
          toggleSelection(nodeId);
        }}
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

function PresetButton({ preset, onClick }: { preset: Preset; onClick: () => void }) {
  return (
    <button class="preset-btn" onClick={onClick} type="button">
      <span>{preset.icon}</span>
      <span>{preset.name}</span>
    </button>
  );
}

export function NodesScreen() {
  const nodes = useAppStore((s) => Array.from(s.nodes.values()));
  const selectedNodeIds = useAppStore((s) => s.selectedNodeIds);
  const presets = useAppStore((s) => s.presets);
  const globalBrightness = useAppStore((s) => s.globalBrightness);
  const setGlobalBrightness = useAppStore((s) => s.setGlobalBrightness);
  const selectAll = useAppStore((s) => s.selectAll);
  const selectNone = useAppStore((s) => s.selectNone);
  const addLogEntry = useAppStore((s) => s.addLogEntry);

  const onlineNodes = nodes.filter((n) => n.status !== 'offline');
  const offlineNodes = nodes.filter((n) => n.status === 'offline');
  const selectedCount = selectedNodeIds.size;

  const handleApplyPreset = async (preset: Preset) => {
    if (selectedCount === 0) {
      addLogEntry('No nodes selected');
      return;
    }

    const nodeIds = Array.from(selectedNodeIds);
    try {
      addLogEntry(`Applying ${preset.name} to ${nodeIds.length} node(s)...`);
      const result = await applyPattern({
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
      addLogEntry(`Error: ${err instanceof Error ? err.message : 'Unknown error'}`);
    }
  };

  const handleApplySelected = async () => {
    if (selectedCount === 0) {
      addLogEntry('No nodes selected');
      return;
    }

    // Apply with the first preset (or rainbow as fallback)
    const defaultPreset = presets.find((p) => p.id === 'preset-rainbow') || presets[0];
    if (defaultPreset) {
      await handleApplyPreset(defaultPreset);
    }
  };

  // Sort: online first, then by name
  const sortedNodes = [...nodes].sort((a, b) => {
    if (a.status === 'offline' && b.status !== 'offline') return 1;
    if (a.status !== 'offline' && b.status === 'offline') return -1;
    return (a.name || a.node_id).localeCompare(b.name || b.node_id);
  });

  return (
    <div class="container-fluid px-0">
      {/* Nodes section */}
      <div class="card mb-4">
        <div class="card-header d-flex justify-content-between align-items-center">
          <span>Nodes</span>
          <button
            class="btn btn-sm btn-outline-primary"
            onClick={() => {
              if (selectedCount === onlineNodes.length) {
                selectNone();
              } else {
                selectAll();
              }
            }}
            type="button"
          >
            {selectedCount === onlineNodes.length ? 'Deselect All' : 'Select All'}
          </button>
        </div>
        <ul class="node-list">
          {sortedNodes.length === 0 ? (
            <li class="node-row text-secondary">
              No nodes discovered yet. Make sure the backend is running.
            </li>
          ) : (
            sortedNodes.map((node) => (
              <NodeRow key={node.node_id} nodeId={node.node_id} />
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
          {offlineNodes.length} offline
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
                onClick={() => handleApplyPreset(preset)}
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
          onInput={(e) => setGlobalBrightness(Number((e.target as HTMLInputElement).value))}
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
