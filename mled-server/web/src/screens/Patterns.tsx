import { useState, useEffect, useCallback, useRef } from 'preact/hooks';
import { useAppStore } from '../store';
import { apiClient, ApiError } from '../lib/apiClient';
import { usePresets, useEditingPreset, useSelectedNodeIds } from '../lib/useStableSelector';
import { EmojiPicker } from '../components/EmojiPicker';
import type { Preset, PatternType, PatternConfig } from '../types';

// Pattern types from MLED/1 protocol (see 0049-NODE-PROTOCOL design doc)
const PATTERN_TYPES: { value: PatternType; label: string; description: string }[] = [
  { value: 'rainbow', label: 'Rainbow', description: 'Rotating hue cycle' },
  { value: 'chase', label: 'Chase', description: 'Moving LED segments' },
  { value: 'breathing', label: 'Breathing', description: 'Pulsing brightness' },
  { value: 'sparkle', label: 'Sparkle', description: 'Random twinkling' },
  { value: 'off', label: 'Off', description: 'All LEDs off' },
];

function getPatternSummary(config: PatternConfig): string {
  const p = config.params as Record<string, unknown>;
  switch (config.type) {
    case 'rainbow':
      return `cycle ${p.speed ?? 10} rpm, sat ${p.saturation ?? 100}%`;
    case 'chase':
      return `chase ${p.speed ?? 30} LED/s, ${p.trains ?? 1} trains`;
    case 'breathing':
      return `breathe ${p.speed ?? 6} bpm, ${p.color ?? '#FFFFFF'}`;
    case 'sparkle':
      return `sparkle ${p.density_pct ?? 30}%, ${p.color_mode ?? 'fixed'}`;
    case 'off':
      return 'all off';
    default:
      return config.type;
  }
}

function PresetListItem({ 
  preset, 
  isSelected, 
  onSelect 
}: { 
  preset: Preset; 
  isSelected: boolean;
  onSelect: () => void;
}) {
  return (
    <li 
      class={`node-row ${isSelected ? 'selected' : ''}`}
      onClick={onSelect}
    >
      <span style={{ fontSize: '1.25rem' }}>{preset.icon}</span>
      <span class="fw-medium flex-grow-1">{preset.name}</span>
      <span class="text-secondary">{getPatternSummary(preset.config)}</span>
    </li>
  );
}

// Generic param change helper
function useParamChange(
  params: Record<string, unknown>,
  onChange: (params: Record<string, unknown>) => void
) {
  return useCallback((key: string, value: unknown) => {
    onChange({ ...params, [key]: value });
  }, [params, onChange]);
}

// Color picker row component
function ColorRow({ 
  label, 
  value, 
  onChange 
}: { 
  label: string; 
  value: string; 
  onChange: (v: string) => void; 
}) {
  return (
    <div class="mb-3">
      <label class="form-label">{label}</label>
      <div class="color-input-wrapper">
        <div class="color-preview" style={{ background: value }} />
        <input
          type="color"
          value={value}
          onInput={(e) => onChange((e.target as HTMLInputElement).value)}
        />
        <input
          type="text"
          class="form-control"
          style={{ maxWidth: '100px' }}
          value={value}
          onInput={(e) => onChange((e.target as HTMLInputElement).value)}
        />
      </div>
    </div>
  );
}

// Slider row component
function SliderRow({ 
  label, 
  value, 
  min, 
  max, 
  unit,
  onChange 
}: { 
  label: string; 
  value: number; 
  min: number; 
  max: number; 
  unit?: string;
  onChange: (v: number) => void; 
}) {
  return (
    <div class="mb-3">
      <label class="form-label">{label}</label>
      <div class="d-flex align-items-center gap-3">
        <input
          type="range"
          class="form-range flex-grow-1"
          min={min}
          max={max}
          value={value}
          onInput={(e) => onChange(Number((e.target as HTMLInputElement).value))}
        />
        <span class="text-primary" style={{ minWidth: '50px' }}>
          {value}{unit || ''}
        </span>
      </div>
    </div>
  );
}

// Select row component
function SelectRow<T extends string>({
  label,
  value,
  options,
  onChange,
}: {
  label: string;
  value: T;
  options: { value: T; label: string }[];
  onChange: (v: T) => void;
}) {
  return (
    <div class="mb-3">
      <label class="form-label">{label}</label>
      <select
        class="form-select"
        value={value}
        onChange={(e) => onChange((e.target as HTMLSelectElement).value as T)}
      >
        {options.map((opt) => (
          <option key={opt.value} value={opt.value}>{opt.label}</option>
        ))}
      </select>
    </div>
  );
}

function PatternParamsEditor({ 
  type, 
  params, 
  onChange 
}: { 
  type: PatternType;
  params: Record<string, unknown>;
  onChange: (params: Record<string, unknown>) => void;
}) {
  const setParam = useParamChange(params, onChange);

  switch (type) {
    case 'rainbow':
      return (
        <>
          <SliderRow 
            label="Speed (rotations/min)" 
            value={(params.speed as number) ?? 10} 
            min={0} max={20} unit=" rpm"
            onChange={(v) => setParam('speed', v)} 
          />
          <SliderRow 
            label="Saturation" 
            value={(params.saturation as number) ?? 100} 
            min={0} max={100} unit="%"
            onChange={(v) => setParam('saturation', v)} 
          />
          <SliderRow 
            label="Color Spread" 
            value={(params.spread_x10 as number) ?? 10} 
            min={1} max={50}
            onChange={(v) => setParam('spread_x10', v)} 
          />
        </>
      );

    case 'chase':
      return (
        <>
          <SliderRow 
            label="Speed (LEDs/sec)" 
            value={(params.speed as number) ?? 30} 
            min={1} max={255}
            onChange={(v) => setParam('speed', v)} 
          />
          <SliderRow 
            label="Tail Length" 
            value={(params.tail_len as number) ?? 5} 
            min={1} max={50}
            onChange={(v) => setParam('tail_len', v)} 
          />
          <SliderRow 
            label="Gap Length" 
            value={(params.gap_len as number) ?? 10} 
            min={0} max={50}
            onChange={(v) => setParam('gap_len', v)} 
          />
          <SliderRow 
            label="Number of Trains" 
            value={(params.trains as number) ?? 1} 
            min={1} max={10}
            onChange={(v) => setParam('trains', v)} 
          />
          <ColorRow 
            label="Foreground Color" 
            value={(params.fg_color as string) ?? '#FFFFFF'} 
            onChange={(v) => setParam('fg_color', v)} 
          />
          <ColorRow 
            label="Background Color" 
            value={(params.bg_color as string) ?? '#000000'} 
            onChange={(v) => setParam('bg_color', v)} 
          />
          <SelectRow
            label="Direction"
            value={(params.direction as 'forward' | 'reverse' | 'bounce') ?? 'forward'}
            options={[
              { value: 'forward', label: 'Forward →' },
              { value: 'reverse', label: '← Reverse' },
              { value: 'bounce', label: '↔ Bounce' },
            ]}
            onChange={(v) => setParam('direction', v)}
          />
          <div class="mb-3 form-check">
            <input
              type="checkbox"
              class="form-check-input"
              id="fade-tail"
              checked={!!params.fade_tail}
              onChange={(e) => setParam('fade_tail', (e.target as HTMLInputElement).checked)}
            />
            <label class="form-check-label" for="fade-tail">Fade tail</label>
          </div>
        </>
      );

    case 'breathing':
      return (
        <>
          <SliderRow 
            label="Speed (breaths/min)" 
            value={(params.speed as number) ?? 6} 
            min={1} max={20} unit=" bpm"
            onChange={(v) => setParam('speed', v)} 
          />
          <ColorRow 
            label="Color" 
            value={(params.color as string) ?? '#FFFFFF'} 
            onChange={(v) => setParam('color', v)} 
          />
          <SliderRow 
            label="Min Brightness" 
            value={(params.min_bri as number) ?? 10} 
            min={0} max={255}
            onChange={(v) => setParam('min_bri', v)} 
          />
          <SliderRow 
            label="Max Brightness" 
            value={(params.max_bri as number) ?? 255} 
            min={0} max={255}
            onChange={(v) => setParam('max_bri', v)} 
          />
          <SelectRow
            label="Curve"
            value={(params.curve as 'sine' | 'linear' | 'ease') ?? 'sine'}
            options={[
              { value: 'sine', label: 'Sine (smooth)' },
              { value: 'linear', label: 'Linear' },
              { value: 'ease', label: 'Ease in/out' },
            ]}
            onChange={(v) => setParam('curve', v)}
          />
        </>
      );

    case 'sparkle':
      return (
        <>
          <SliderRow 
            label="Speed (spawn rate)" 
            value={(params.speed as number) ?? 10} 
            min={1} max={20}
            onChange={(v) => setParam('speed', v)} 
          />
          <ColorRow 
            label="Sparkle Color" 
            value={(params.color as string) ?? '#FFFFFF'} 
            onChange={(v) => setParam('color', v)} 
          />
          <SliderRow 
            label="Density" 
            value={(params.density_pct as number) ?? 30} 
            min={1} max={100} unit="%"
            onChange={(v) => setParam('density_pct', v)} 
          />
          <SliderRow 
            label="Fade Speed" 
            value={(params.fade_speed as number) ?? 50} 
            min={1} max={255}
            onChange={(v) => setParam('fade_speed', v)} 
          />
          <SelectRow
            label="Color Mode"
            value={(params.color_mode as 'fixed' | 'random' | 'rainbow') ?? 'fixed'}
            options={[
              { value: 'fixed', label: 'Fixed Color' },
              { value: 'random', label: 'Random Colors' },
              { value: 'rainbow', label: 'Rainbow Colors' },
            ]}
            onChange={(v) => setParam('color_mode', v)}
          />
          <ColorRow 
            label="Background Color" 
            value={(params.bg_color as string) ?? '#000000'} 
            onChange={(v) => setParam('bg_color', v)} 
          />
        </>
      );

    case 'off':
    default:
      return <p class="text-secondary">All LEDs will be turned off.</p>;
  }
}

function PresetEditor({ 
  preset, 
  isNew,
  canPreview,
  onSave,
  onCancel,
  onDelete,
  onPreview,
}: { 
  preset: Preset;
  isNew: boolean;
  canPreview: boolean;
  onSave: (preset: Preset) => void;
  onCancel: () => void;
  onDelete: () => void;
  onPreview: (preset: Preset) => void;
}) {
  const [name, setName] = useState(preset.name);
  const [icon, setIcon] = useState(preset.icon);
  const [type, setType] = useState<PatternType>(preset.config.type);
  const [brightness, setBrightness] = useState(preset.config.brightness);
  const [params, setParams] = useState(preset.config.params);
  const [pinPreview, setPinPreview] = useState(false);
  const previewTimeoutRef = useRef<number | null>(null);
  const PREVIEW_DEBOUNCE_MS = 350;

  // Reset when preset changes
  useEffect(() => {
    setName(preset.name);
    setIcon(preset.icon);
    setType(preset.config.type);
    setBrightness(preset.config.brightness);
    setParams(preset.config.params);
  }, [preset.id]);

  const buildPreset = useCallback((): Preset => ({
    ...preset,
    name,
    icon,
    config: {
      type,
      brightness,
      params,
    },
  }), [preset, name, icon, type, brightness, params]);

  const handleTypeChange = useCallback((e: Event) => {
    const newType = (e.target as HTMLSelectElement).value as PatternType;
    setType(newType);
    // Reset params to defaults for new type (from MLED/1 protocol)
    switch (newType) {
      case 'rainbow':
        setParams({ 
          speed: 10,           // 10 rotations/min
          saturation: 100,     // full saturation
          spread_x10: 10,      // spread = 1.0
        });
        break;
      case 'chase':
        setParams({ 
          speed: 30,           // 30 LEDs/sec
          tail_len: 5,
          gap_len: 10,
          trains: 1,
          fg_color: '#FFFFFF',
          bg_color: '#000000',
          direction: 'forward',
          fade_tail: true,
        });
        break;
      case 'breathing':
        setParams({ 
          speed: 6,            // 6 breaths/min
          color: '#FFFFFF',
          min_bri: 10,
          max_bri: 255,
          curve: 'sine',
        });
        break;
      case 'sparkle':
        setParams({ 
          speed: 10,           // spawn rate
          color: '#FFFFFF',
          density_pct: 30,
          fade_speed: 50,
          color_mode: 'fixed',
          bg_color: '#000000',
        });
        break;
      case 'off':
        setParams({});
        break;
    }
  }, []);

  const handleSaveClick = useCallback(() => {
    onSave(buildPreset());
  }, [onSave, buildPreset]);

  const handlePreviewClick = useCallback(() => {
    onPreview(buildPreset());
  }, [onPreview, buildPreset]);

  const handleNameChange = useCallback((e: Event) => {
    setName((e.target as HTMLInputElement).value);
  }, []);

  const handleIconChange = useCallback((newIcon: string) => {
    setIcon(newIcon);
  }, []);

  const handleBrightnessChange = useCallback((e: Event) => {
    setBrightness(Number((e.target as HTMLInputElement).value));
  }, []);

  const handlePinPreviewChange = useCallback((e: Event) => {
    setPinPreview((e.target as HTMLInputElement).checked);
  }, []);

  useEffect(() => {
    if (!pinPreview || !canPreview) {
      if (previewTimeoutRef.current !== null) {
        window.clearTimeout(previewTimeoutRef.current);
        previewTimeoutRef.current = null;
      }
      return;
    }

    if (previewTimeoutRef.current !== null) {
      window.clearTimeout(previewTimeoutRef.current);
    }
    previewTimeoutRef.current = window.setTimeout(() => {
      onPreview(buildPreset());
      previewTimeoutRef.current = null;
    }, PREVIEW_DEBOUNCE_MS);

    return () => {
      if (previewTimeoutRef.current !== null) {
        window.clearTimeout(previewTimeoutRef.current);
        previewTimeoutRef.current = null;
      }
    };
  }, [pinPreview, canPreview, type, brightness, params, onPreview, buildPreset]);

  return (
    <div class="card">
      <div class="card-header">
        {isNew ? 'New Preset' : `Edit: ${preset.name}`}
      </div>
      <div class="card-body">
        <div class="mb-3">
          <label class="form-label">Name</label>
          <input
            type="text"
            class="form-control"
            value={name}
            onInput={handleNameChange}
          />
        </div>

        <div class="mb-3">
          <label class="form-label">Icon</label>
          <EmojiPicker value={icon} onChange={handleIconChange} />
        </div>

        <div class="mb-3">
          <label class="form-label">Type</label>
          <select
            class="form-select"
            value={type}
            onChange={handleTypeChange}
          >
            {PATTERN_TYPES.map((pt) => (
              <option key={pt.value} value={pt.value}>{pt.label}</option>
            ))}
          </select>
        </div>

        <PatternParamsEditor
          type={type}
          params={params}
          onChange={setParams}
        />

        <div class="mb-4">
          <label class="form-label">Brightness</label>
          <div class="d-flex align-items-center gap-3">
            <input
              type="range"
              class="form-range flex-grow-1"
              min="0"
              max="100"
              value={brightness}
              onInput={handleBrightnessChange}
            />
            <span class="text-primary" style={{ minWidth: '40px' }}>{brightness}%</span>
          </div>
        </div>

        <div class="d-flex gap-2">
          <div class="form-check d-flex align-items-center me-2">
            <input
              type="checkbox"
              class="form-check-input"
              id="pin-preview"
              checked={pinPreview}
              disabled={!canPreview}
              onChange={handlePinPreviewChange}
            />
            <label class="form-check-label ms-1" for="pin-preview">
              Pin preview (auto-apply)
            </label>
          </div>
          <button
            class="btn btn-outline-primary"
            onClick={handlePreviewClick}
            type="button"
          >
            👁️ Preview on Selected
          </button>
          <button
            class="btn btn-primary"
            onClick={handleSaveClick}
            type="button"
          >
            💾 Save
          </button>
          {!isNew && (
            <button
              class="btn btn-danger"
              onClick={onDelete}
              type="button"
            >
              🗑️ Delete
            </button>
          )}
          <button
            class="btn btn-secondary ms-auto"
            onClick={onCancel}
            type="button"
          >
            Cancel
          </button>
        </div>
      </div>
    </div>
  );
}

export function PatternsScreen() {
  // Use stable selector hooks
  const presets = usePresets();
  const editingPreset = useEditingPreset();
  const selectedNodeIdsArray = useSelectedNodeIds();
  
  // Direct store access for actions
  const editingPresetId = useAppStore((s) => s.editingPresetId);
  const setEditingPresetId = useAppStore((s) => s.setEditingPresetId);
  const storeAddPreset = useAppStore((s) => s.addPreset);
  const storeUpdatePreset = useAppStore((s) => s.updatePreset);
  const storeDeletePreset = useAppStore((s) => s.deletePreset);
  const addLogEntry = useAppStore((s) => s.addLogEntry);

  const [isCreatingNew, setIsCreatingNew] = useState(false);
  const [tempPreset, setTempPreset] = useState<Preset | null>(null);

  const handleNewPreset = useCallback(() => {
    const newPreset: Preset = {
      id: `preset-${Date.now()}`,
      name: 'New Preset',
      icon: '✨',
      config: {
        type: 'solid',
        brightness: 75,
        params: { color: '#2244AA' },
      },
    };
    setTempPreset(newPreset);
    setIsCreatingNew(true);
    setEditingPresetId(null);
  }, [setEditingPresetId]);

  const handleSelectPreset = useCallback((id: string) => {
    setIsCreatingNew(false);
    setTempPreset(null);
    setEditingPresetId(id);
  }, [setEditingPresetId]);

  const handleSave = useCallback(async (preset: Preset) => {
    try {
      if (isCreatingNew) {
        await apiClient.createPreset(preset);
        storeAddPreset(preset);
        addLogEntry(`Created preset: ${preset.name}`);
      } else {
        await apiClient.updatePreset(preset);
        storeUpdatePreset(preset);
        addLogEntry(`Updated preset: ${preset.name}`);
      }
      setIsCreatingNew(false);
      setTempPreset(null);
      setEditingPresetId(preset.id);
    } catch (err) {
      addLogEntry(`Error saving preset: ${err instanceof Error ? err.message : 'Unknown'}`);
    }
  }, [isCreatingNew, storeAddPreset, storeUpdatePreset, addLogEntry, setEditingPresetId]);

  const handleDelete = useCallback(async () => {
    if (!editingPreset) return;
    
    if (confirm(`Delete preset "${editingPreset.name}"?`)) {
      try {
        await apiClient.deletePreset(editingPreset.id);
        storeDeletePreset(editingPreset.id);
        addLogEntry(`Deleted preset: ${editingPreset.name}`);
        setEditingPresetId(null);
      } catch (err) {
        addLogEntry(`Error deleting preset: ${err instanceof Error ? err.message : 'Unknown'}`);
      }
    }
  }, [editingPreset, storeDeletePreset, addLogEntry, setEditingPresetId]);

  const handleCancel = useCallback(() => {
    setIsCreatingNew(false);
    setTempPreset(null);
    setEditingPresetId(null);
  }, [setEditingPresetId]);

  const handlePreview = useCallback(async (preset: Preset) => {
    if (selectedNodeIdsArray.length === 0) {
      addLogEntry('No nodes selected for preview');
      return;
    }

    try {
      addLogEntry(`Previewing ${preset.name}...`);
      await apiClient.applyPattern({
        node_ids: selectedNodeIdsArray,
        config: preset.config,
      });
      addLogEntry('Preview applied');
    } catch (err) {
      if (err instanceof ApiError && err.status === 400) {
        addLogEntry(`Preview rejected (400): ${err.message}`);
        return;
      }
      addLogEntry(`Preview error: ${err instanceof Error ? err.message : 'Unknown'}`);
    }
  }, [selectedNodeIdsArray, addLogEntry]);

  const currentPreset = isCreatingNew ? tempPreset : editingPreset;
  const canPreview = selectedNodeIdsArray.length > 0;

  return (
    <div class="container-fluid px-0">
      {/* Presets list */}
      <div class="card mb-4">
        <div class="card-header d-flex justify-content-between align-items-center">
          <span>Presets</span>
          <button
            class="btn btn-sm btn-primary"
            onClick={handleNewPreset}
            type="button"
          >
            + New
          </button>
        </div>
        <ul class="node-list">
          {presets.map((preset) => (
            <PresetListItem
              key={preset.id}
              preset={preset}
              isSelected={editingPresetId === preset.id}
              onSelect={() => handleSelectPreset(preset.id)}
            />
          ))}
        </ul>
      </div>

      {/* Editor */}
      {currentPreset && (
        <PresetEditor
          preset={currentPreset}
          isNew={isCreatingNew}
          canPreview={canPreview}
          onSave={handleSave}
          onCancel={handleCancel}
          onDelete={handleDelete}
          onPreview={handlePreview}
        />
      )}

      {/* Spacer for bottom nav */}
      <div style={{ height: '80px' }} />
    </div>
  );
}
