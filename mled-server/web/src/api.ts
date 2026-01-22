/**
 * API layer - re-exports from apiClient and provides loadInitialData
 * 
 * This file is kept for backwards compatibility. New code should import
 * directly from './lib/apiClient'.
 */

import { useAppStore } from './store';
import { apiClient } from './lib/apiClient';

// Re-export all API methods for backwards compatibility
export const {
  fetchNodes,
  discoverNodes,
  pingNodes,
  applyPattern,
  fetchPresets,
  createPreset,
  updatePreset,
  deletePreset,
  fetchStatus,
  fetchSettings,
  updateSettings,
  fetchNetworkInterfaces,
} = apiClient;

/**
 * Load all initial data in parallel and update the store.
 * Called on app mount and periodically.
 */
export async function loadInitialData(): Promise<void> {
  const store = useAppStore.getState();

  try {
    // Load all data in parallel, with individual error handling
    const [nodes, presets, settings, status] = await Promise.all([
      apiClient.fetchNodes().catch(() => []),
      apiClient.fetchPresets().catch(() => null),
      apiClient.fetchSettings().catch(() => null),
      apiClient.fetchStatus().catch(() => null),
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
  } catch (error) {
    store.setConnected(false);
    store.setLastError(error instanceof Error ? error.message : 'Failed to load data');
  }
}
