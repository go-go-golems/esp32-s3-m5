import { useState, useEffect, useCallback } from 'preact/hooks';
import { useAppStore } from '../store';
import { apiClient } from '../lib/apiClient';
import { usePresets } from '../lib/useStableSelector';
import type { Settings, NetworkInterface } from '../types';

export function SettingsModal({ onClose }: { onClose: () => void }) {
  const settings = useAppStore((s) => s.settings);
  const presets = usePresets();
  const setSettings = useAppStore((s) => s.setSettings);
  const setPresets = useAppStore((s) => s.setPresets);
  const addLogEntry = useAppStore((s) => s.addLogEntry);

  const [localSettings, setLocalSettings] = useState<Settings>({ ...settings });
  const [interfaces, setInterfaces] = useState<NetworkInterface[]>([]);
  const [saving, setSaving] = useState(false);

  // Load network interfaces
  useEffect(() => {
    apiClient.fetchNetworkInterfaces()
      .then(setInterfaces)
      .catch(() => {
        // Fallback if API not available
        setInterfaces([{ name: 'auto', ip: 'auto' }]);
      });
  }, []);

  const handleChange = useCallback(<K extends keyof Settings>(key: K, value: Settings[K]) => {
    setLocalSettings((prev) => ({ ...prev, [key]: value }));
  }, []);

  const handleSave = useCallback(async () => {
    setSaving(true);
    try {
      const updated = await apiClient.updateSettings(localSettings);
      setSettings(updated);
      addLogEntry('Settings saved');
      onClose();
    } catch (err) {
      addLogEntry(`Error saving settings: ${err instanceof Error ? err.message : 'Unknown'}`);
    } finally {
      setSaving(false);
    }
  }, [localSettings, setSettings, addLogEntry, onClose]);

  const handleExportPresets = useCallback(() => {
    const data = JSON.stringify(presets, null, 2);
    const blob = new Blob([data], { type: 'application/json' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = 'mled-presets.json';
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
    URL.revokeObjectURL(url);
    addLogEntry('Presets exported');
  }, [presets, addLogEntry]);

  const handleImportPresets = useCallback(() => {
    const input = document.createElement('input');
    input.type = 'file';
    input.accept = '.json';
    input.onchange = async (e) => {
      const file = (e.target as HTMLInputElement).files?.[0];
      if (!file) return;

      try {
        const text = await file.text();
        const imported = JSON.parse(text);
        if (Array.isArray(imported)) {
          setPresets(imported);
          addLogEntry(`Imported ${imported.length} preset(s)`);
        } else {
          throw new Error('Invalid preset file format');
        }
      } catch (err) {
        addLogEntry(`Import error: ${err instanceof Error ? err.message : 'Unknown'}`);
      }
    };
    input.click();
  }, [setPresets, addLogEntry]);

  return (
    <div class="modal d-block" style={{ background: 'rgba(0,0,0,0.7)' }}>
      <div class="modal-dialog modal-lg modal-dialog-centered">
        <div class="modal-content">
          <div class="modal-header">
            <h5 class="modal-title">🎛️ MLED Controller › Settings</h5>
            <button
              type="button"
              class="btn-close"
              onClick={onClose}
              aria-label="Close"
            />
          </div>
          <div class="modal-body">
            {/* Network settings */}
            <div class="settings-section">
              <h5>Network</h5>
              
              <div class="mb-3">
                <label class="form-label">Bind IP</label>
                <select
                  class="form-select"
                  value={localSettings.bind_ip}
                  onChange={(e) => handleChange('bind_ip', (e.target as HTMLSelectElement).value)}
                >
                  <option value="auto">Auto</option>
                  {interfaces.map((iface) => (
                    <option key={iface.name} value={iface.ip}>
                      {iface.name}: {iface.ip}
                    </option>
                  ))}
                </select>
              </div>

              <div class="row">
                <div class="col-8">
                  <label class="form-label">Multicast Group</label>
                  <input
                    type="text"
                    class="form-control"
                    value={localSettings.multicast_group}
                    onInput={(e) => handleChange('multicast_group', (e.target as HTMLInputElement).value)}
                  />
                </div>
                <div class="col-4">
                  <label class="form-label">Port</label>
                  <input
                    type="number"
                    class="form-control"
                    value={localSettings.multicast_port}
                    onInput={(e) => handleChange('multicast_port', Number((e.target as HTMLInputElement).value))}
                  />
                </div>
              </div>
            </div>

            {/* Timing settings */}
            <div class="settings-section">
              <h5>Timing</h5>
              
              <div class="row">
                <div class="col-6">
                  <label class="form-label">Discovery interval (ms)</label>
                  <input
                    type="number"
                    class="form-control"
                    value={localSettings.discovery_interval_ms}
                    onInput={(e) => handleChange('discovery_interval_ms', Number((e.target as HTMLInputElement).value))}
                  />
                </div>
                <div class="col-6">
                  <label class="form-label">Offline threshold (seconds)</label>
                  <input
                    type="number"
                    class="form-control"
                    value={localSettings.offline_threshold_s}
                    onInput={(e) => handleChange('offline_threshold_s', Number((e.target as HTMLInputElement).value))}
                  />
                </div>
              </div>
            </div>

            {/* Presets section */}
            <div class="settings-section mb-0">
              <h5>Presets</h5>
              
              <div class="d-flex gap-2">
                <button
                  class="btn btn-outline-primary"
                  onClick={handleExportPresets}
                  type="button"
                >
                  📤 Export All
                </button>
                <button
                  class="btn btn-outline-primary"
                  onClick={handleImportPresets}
                  type="button"
                >
                  📥 Import
                </button>
              </div>
            </div>
          </div>
          <div class="modal-footer">
            <button
              type="button"
              class="btn btn-secondary"
              onClick={onClose}
            >
              Cancel
            </button>
            <button
              type="button"
              class="btn btn-primary"
              onClick={handleSave}
              disabled={saving}
            >
              {saving ? 'Saving...' : '💾 Save'}
            </button>
          </div>
        </div>
      </div>
    </div>
  );
}
