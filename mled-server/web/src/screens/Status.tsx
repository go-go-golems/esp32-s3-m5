import { useMemo, useCallback } from 'preact/hooks';
import { useAppStore } from '../store';
import { apiClient } from '../lib/apiClient';
import { useNodes, useOnlineNodes, useSortedNodes } from '../lib/useStableSelector';
import type { Node } from '../types';

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

function formatDuration(ms: number): string {
  const seconds = Math.floor(ms / 1000);
  const minutes = Math.floor(seconds / 60);
  if (minutes < 1) return 'just now';
  if (minutes < 60) return `${minutes}m ago`;
  const hours = Math.floor(minutes / 60);
  return `${hours}h ${minutes % 60}m ago`;
}

function getPatternLabel(node: Node): string {
  if (!node.current_pattern) return '---';
  switch (node.current_pattern.type) {
    case 'rainbow': return 'Rainbow';
    case 'solid': return 'Solid';
    case 'gradient': return 'Gradient';
    case 'pulse': return 'Pulse';
    case 'off': return 'Off';
    default: return node.current_pattern.type;
  }
}

interface Problem {
  type: 'warning' | 'error';
  icon: string;
  nodeName: string;
  message: string;
}

function detectProblems(nodes: Node[]): Problem[] {
  const problems: Problem[] = [];

  for (const node of nodes) {
    if (node.status === 'offline') {
      const duration = formatDuration(Date.now() - node.last_seen);
      problems.push({
        type: 'error',
        icon: '❌',
        nodeName: node.name || node.node_id,
        message: `offline for ${duration}`,
      });
    } else if (node.status === 'weak' || node.rssi < -70) {
      problems.push({
        type: 'warning',
        icon: '⚠️',
        nodeName: node.name || node.node_id,
        message: `weak signal (${formatRssi(node.rssi)}), may drop`,
      });
    }
  }

  return problems;
}

function SystemBanner() {
  const nodes = useNodes();
  const onlineNodes = useOnlineNodes();
  const status = useAppStore((s) => s.status);

  const problematicNodes = useMemo(
    () => nodes.filter((n) => n.status === 'offline' || n.status === 'weak'),
    [nodes]
  );

  let healthStatus: 'healthy' | 'degraded' | 'offline';
  let healthIcon: string;
  let healthLabel: string;

  if (!status.connected) {
    healthStatus = 'offline';
    healthIcon = '❌';
    healthLabel = 'Disconnected';
  } else if (problematicNodes.length === 0) {
    healthStatus = 'healthy';
    healthIcon = '✅';
    healthLabel = 'Healthy';
  } else if (onlineNodes.length > 0) {
    healthStatus = 'degraded';
    healthIcon = '⚠️';
    healthLabel = 'Degraded';
  } else {
    healthStatus = 'offline';
    healthIcon = '❌';
    healthLabel = 'All Offline';
  }

  return (
    <div class={`status-banner ${healthStatus} mb-4`}>
      <div style={{ fontSize: '2rem' }}>{healthIcon}</div>
      <div class="flex-grow-1">
        <h4 class="mb-1">System Status: {healthLabel}</h4>
        <div class="text-secondary">
          <div>Controller: {status.bindIp || 'auto'} ({status.connected ? 'connected' : 'disconnected'})</div>
          <div>Multicast: {status.multicastGroup}:{status.multicastPort}</div>
          <div>Nodes: {onlineNodes.length}/{nodes.length} online</div>
        </div>
      </div>
    </div>
  );
}

function ProblemList() {
  const nodes = useNodes();
  const problems = useMemo(() => detectProblems(nodes), [nodes]);

  if (problems.length === 0) {
    return (
      <div class="card mb-4">
        <div class="card-header">Problems</div>
        <div class="card-body text-secondary">
          <span style={{ fontSize: '1.25rem', marginRight: '8px' }}>✓</span>
          No problems detected
        </div>
      </div>
    );
  }

  return (
    <div class="card mb-4">
      <div class="card-header">Problems ({problems.length})</div>
      <div class="card-body p-2">
        {problems.map((problem, idx) => (
          <div key={idx} class={`problem-item ${problem.type}`}>
            <span style={{ fontSize: '1.25rem' }}>{problem.icon}</span>
            <span class="fw-medium">{problem.nodeName}</span>
            <span class="text-secondary">— {problem.message}</span>
          </div>
        ))}
      </div>
    </div>
  );
}

function NodeTable() {
  const sortedNodes = useSortedNodes();

  return (
    <div class="card mb-4">
      <div class="card-header">Node Details</div>
      <div class="table-responsive">
        <table class="table table-dark table-hover mb-0">
          <thead>
            <tr>
              <th>Node</th>
              <th>Signal</th>
              <th>Uptime</th>
              <th>Pattern</th>
              <th>Status</th>
            </tr>
          </thead>
          <tbody>
            {sortedNodes.length === 0 ? (
              <tr>
                <td colSpan={5} class="text-secondary text-center">
                  No nodes discovered
                </td>
              </tr>
            ) : (
              sortedNodes.map((node) => (
                <tr key={node.node_id}>
                  <td class="fw-medium">{node.name || node.node_id}</td>
                  <td>{formatRssi(node.rssi)}</td>
                  <td>{formatUptime(node.uptime_ms)}</td>
                  <td>{getPatternLabel(node)}</td>
                  <td>
                    <span class={`badge bg-${
                      node.status === 'online' ? 'success' :
                      node.status === 'weak' ? 'warning' : 'danger'
                    }`}>
                      {node.status === 'online' ? '🟢 good' :
                       node.status === 'weak' ? '🟡 weak' : '🔴 off'}
                    </span>
                  </td>
                </tr>
              ))
            )}
          </tbody>
        </table>
      </div>
    </div>
  );
}

export function StatusScreen() {
  const setShowSettings = useAppStore((s) => s.setShowSettings);
  const setNodes = useAppStore((s) => s.setNodes);
  const addLogEntry = useAppStore((s) => s.addLogEntry);

  const handleRefresh = useCallback(async () => {
    try {
      addLogEntry('Refreshing nodes...');
      const nodes = await apiClient.discoverNodes(1000);
      setNodes(nodes);
      addLogEntry(`Discovered ${nodes.length} node(s)`);
    } catch (err) {
      addLogEntry(`Refresh error: ${err instanceof Error ? err.message : 'Unknown'}`);
    }
  }, [addLogEntry, setNodes]);

  const handleOpenSettings = useCallback(() => {
    setShowSettings(true);
  }, [setShowSettings]);

  return (
    <div class="container-fluid px-0">
      <SystemBanner />
      <ProblemList />
      <NodeTable />

      {/* Action buttons */}
      <div class="d-flex gap-3">
        <button
          class="btn btn-primary"
          onClick={handleRefresh}
          type="button"
        >
          🔄 Refresh All
        </button>
        <button
          class="btn btn-secondary ms-auto"
          onClick={handleOpenSettings}
          type="button"
        >
          ⚙️ Settings
        </button>
      </div>

      {/* Spacer for bottom nav */}
      <div style={{ height: '80px' }} />
    </div>
  );
}
