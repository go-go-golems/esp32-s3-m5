/**
 * Pattern 1: Stable Selectors for Zustand
 * 
 * Problem: Zustand triggers re-renders when selector returns a new reference.
 * Creating arrays/objects inside selectors (e.g., Array.from(map.values()))
 * causes infinite render loops.
 * 
 * Solution: Select the raw data (Map/Set) and derive arrays in useMemo outside
 * the selector. This file provides helper hooks that encapsulate this pattern.
 */

import { useMemo } from 'preact/hooks';
import { useAppStore } from '../store';
import type { Node, Preset } from '../types';

/**
 * Get all nodes as a stable array.
 * The array reference only changes when the underlying Map changes.
 */
export function useNodes(): Node[] {
  const nodesMap = useAppStore((s) => s.nodes);
  return useMemo(
    () => (nodesMap ? Array.from(nodesMap.values()) : []),
    [nodesMap]
  );
}

/**
 * Get nodes filtered by status.
 */
export function useNodesByStatus(status: 'online' | 'weak' | 'offline'): Node[] {
  const nodes = useNodes();
  return useMemo(
    () => nodes.filter((n) => n.status === status),
    [nodes, status]
  );
}

/**
 * Get online nodes (status !== 'offline').
 */
export function useOnlineNodes(): Node[] {
  const nodes = useNodes();
  return useMemo(
    () => nodes.filter((n) => n.status !== 'offline'),
    [nodes]
  );
}

/**
 * Get nodes sorted for display (online first, then by name).
 */
export function useSortedNodes(): Node[] {
  const nodes = useNodes();
  return useMemo(() => {
    return [...nodes].sort((a, b) => {
      // Offline nodes last
      if (a.status === 'offline' && b.status !== 'offline') return 1;
      if (a.status !== 'offline' && b.status === 'offline') return -1;
      // Then by name
      return (a.name || a.node_id).localeCompare(b.name || b.node_id);
    });
  }, [nodes]);
}

/**
 * Get node IDs from a sorted list (for rendering with stable keys).
 */
export function useSortedNodeIds(): string[] {
  const sortedNodes = useSortedNodes();
  return useMemo(
    () => sortedNodes.map((n) => n.node_id),
    [sortedNodes]
  );
}

/**
 * Get selected node IDs as an array.
 */
export function useSelectedNodeIds(): string[] {
  const selectedSet = useAppStore((s) => s.selectedNodeIds);
  return useMemo(
    () => (selectedSet ? Array.from(selectedSet) : []),
    [selectedSet]
  );
}

/**
 * Get the count of selected nodes (stable primitive, no memoization needed).
 */
export function useSelectedCount(): number {
  const selectedSet = useAppStore((s) => s.selectedNodeIds);
  return selectedSet?.size ?? 0;
}

/**
 * Check if a specific node is selected.
 */
export function useIsNodeSelected(nodeId: string): boolean {
  return useAppStore((s) => s.selectedNodeIds?.has(nodeId) ?? false);
}

/**
 * Get presets, with fallback to empty array.
 */
export function usePresets(): Preset[] {
  const presets = useAppStore((s) => s.presets);
  return presets ?? [];
}

/**
 * Get the currently editing preset.
 */
export function useEditingPreset(): Preset | null {
  const presets = usePresets();
  const editingId = useAppStore((s) => s.editingPresetId);
  return useMemo(
    () => presets.find((p) => p.id === editingId) ?? null,
    [presets, editingId]
  );
}
