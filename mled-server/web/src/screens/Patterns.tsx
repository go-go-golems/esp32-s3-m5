import { useState, useEffect } from 'preact/hooks';
import { useAppStore, useEditingPreset } from '../store';
import { applyPattern, createPreset, updatePreset, deletePreset as apiDeletePreset } from '../api';
import type { Preset, PatternType, PatternConfig } from '../types';

const PATTERN_TYPES: { value: PatternType; label: string }[] = [
  { value: 'rainbow', label: 'Rainbow Cycle' },
  { value: 'solid', label: 'Solid Color' },
  { value: 'gradient', label: 'Gradient' },
  { value: 'pulse', label: 'Pulse' },
  { value: 'off', label: 'Off' },
];

function getPatternSummary(config: PatternConfig): string {
  switch (config.type) {
    case 'rainbow':
      return `cycle, speed ${config.params.speed || 50}`;
    case 'solid':
      return `solid ${config.params.color || '#FFFFFF'}, ${config.brightness}%`;
    case 'gradient':
      return `gradient ${config.params.color_start}→${config.params.color_end}`;
    case 'pulse':
      return `strobe ${config.params.color}, ${config.params.speed || 50}Hz`;
    case 'off':
      return 'solid black';
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

function PatternParamsEditor({ 
  type, 
  params, 
  onChange 
}: { 
  type: PatternType;
  params: Record<string, number | string>;
  onChange: (params: Record<string, number | string>) => void;
}) {
  switch (type) {
    case 'rainbow':
      return (
        <div class="mb-3">
          <label class="form-label">Speed</label>
          <div class="d-flex align-items-center gap-3">
            <input
              type="range"
              class="form-range flex-grow-1"
              min="0"
              max="100"
              value={params.speed as number || 50}
              onInput={(e) => onChange({ ...params, speed: Number((e.target as HTMLInputElement).value) })}
            />
            <span class="text-primary" style={{ minWidth: '40px' }}>{params.speed || 50}</span>
          </div>
        </div>
      );

    case 'solid':
      return (
        <div class="mb-3">
          <label class="form-label">Color</label>
          <div class="color-input-wrapper">
            <div 
              class="color-preview" 
              style={{ background: params.color as string || '#FFFFFF' }} 
            />
            <input
              type="color"
              value={params.color as string || '#FFFFFF'}
              onInput={(e) => onChange({ ...params, color: (e.target as HTMLInputElement).value })}
            />
            <input
              type="text"
              class="form-control"
              style={{ maxWidth: '120px' }}
              value={params.color as string || '#FFFFFF'}
              onInput={(e) => onChange({ ...params, color: (e.target as HTMLInputElement).value })}
            />
          </div>
        </div>
      );

    case 'gradient':
      return (
        <>
          <div class="mb-3">
            <label class="form-label">Start Color</label>
            <div class="color-input-wrapper">
              <div 
                class="color-preview" 
                style={{ background: params.color_start as string || '#FF6B35' }} 
              />
              <input
                type="color"
                value={params.color_start as string || '#FF6B35'}
                onInput={(e) => onChange({ ...params, color_start: (e.target as HTMLInputElement).value })}
              />
            </div>
          </div>
          <div class="mb-3">
            <label class="form-label">End Color</label>
            <div class="color-input-wrapper">
              <div 
                class="color-preview" 
                style={{ background: params.color_end as string || '#AA2200' }} 
              />
              <input
                type="color"
                value={params.color_end as string || '#AA2200'}
                onInput={(e) => onChange({ ...params, color_end: (e.target as HTMLInputElement).value })}
              />
            </div>
          </div>
        </>
      );

    case 'pulse':
      return (
        <>
          <div class="mb-3">
            <label class="form-label">Color</label>
            <div class="color-input-wrapper">
              <div 
                class="color-preview" 
                style={{ background: params.color as string || '#FFFFFF' }} 
              />
              <input
                type="color"
                value={params.color as string || '#FFFFFF'}
                onInput={(e) => onChange({ ...params, color: (e.target as HTMLInputElement).value })}
              />
            </div>
          </div>
          <div class="mb-3">
            <label class="form-label">Speed</label>
            <div class="d-flex align-items-center gap-3">
              <input
                type="range"
                class="form-range flex-grow-1"
                min="0"
                max="100"
                value={params.speed as number || 60}
                onInput={(e) => onChange({ ...params, speed: Number((e.target as HTMLInputElement).value) })}
              />
              <span class="text-primary" style={{ minWidth: '40px' }}>{params.speed || 60}</span>
            </div>
          </div>
        </>
      );

    case 'off':
    default:
      return <p class="text-secondary">No parameters for this pattern type.</p>;
  }
}

function PresetEditor({ 
  preset, 
  isNew,
  onSave,
  onCancel,
  onDelete,
  onPreview,
}: { 
  preset: Preset;
  isNew: boolean;
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

  // Reset when preset changes
  useEffect(() => {
    setName(preset.name);
    setIcon(preset.icon);
    setType(preset.config.type);
    setBrightness(preset.config.brightness);
    setParams(preset.config.params);
  }, [preset.id]);

  const buildPreset = (): Preset => ({
    ...preset,
    name,
    icon,
    config: {
      type,
      brightness,
      params,
    },
  });

  const handleTypeChange = (newType: PatternType) => {
    setType(newType);
    // Reset params to defaults for new type
    switch (newType) {
      case 'rainbow':
        setParams({ speed: 50 });
        break;
      case 'solid':
        setParams({ color: '#2244AA' });
        break;
      case 'gradient':
        setParams({ color_start: '#FF6B35', color_end: '#AA2200' });
        break;
      case 'pulse':
        setParams({ color: '#FFFFFF', speed: 60 });
        break;
      case 'off':
        setParams({});
        break;
    }
  };

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
            onInput={(e) => setName((e.target as HTMLInputElement).value)}
          />
        </div>

        <div class="mb-3">
          <label class="form-label">Icon</label>
          <input
            type="text"
            class="form-control"
            style={{ maxWidth: '80px' }}
            value={icon}
            onInput={(e) => setIcon((e.target as HTMLInputElement).value)}
          />
        </div>

        <div class="mb-3">
          <label class="form-label">Type</label>
          <select
            class="form-select"
            value={type}
            onChange={(e) => handleTypeChange((e.target as HTMLSelectElement).value as PatternType)}
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
              onInput={(e) => setBrightness(Number((e.target as HTMLInputElement).value))}
            />
            <span class="text-primary" style={{ minWidth: '40px' }}>{brightness}%</span>
          </div>
        </div>

        <div class="d-flex gap-2">
          <button
            class="btn btn-outline-primary"
            onClick={() => onPreview(buildPreset())}
            type="button"
          >
            👁️ Preview on Selected
          </button>
          <button
            class="btn btn-primary"
            onClick={() => onSave(buildPreset())}
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
  const presets = useAppStore((s) => s.presets);
  const editingPresetId = useAppStore((s) => s.editingPresetId);
  const setEditingPresetId = useAppStore((s) => s.setEditingPresetId);
  const storeAddPreset = useAppStore((s) => s.addPreset);
  const storeUpdatePreset = useAppStore((s) => s.updatePreset);
  const storeDeletePreset = useAppStore((s) => s.deletePreset);
  const selectedNodeIds = useAppStore((s) => s.selectedNodeIds);
  const addLogEntry = useAppStore((s) => s.addLogEntry);

  const editingPreset = useEditingPreset();
  const [isCreatingNew, setIsCreatingNew] = useState(false);
  const [tempPreset, setTempPreset] = useState<Preset | null>(null);

  const handleNewPreset = () => {
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
  };

  const handleSelectPreset = (id: string) => {
    setIsCreatingNew(false);
    setTempPreset(null);
    setEditingPresetId(id);
  };

  const handleSave = async (preset: Preset) => {
    try {
      if (isCreatingNew) {
        await createPreset(preset);
        storeAddPreset(preset);
        addLogEntry(`Created preset: ${preset.name}`);
      } else {
        await updatePreset(preset);
        storeUpdatePreset(preset);
        addLogEntry(`Updated preset: ${preset.name}`);
      }
      setIsCreatingNew(false);
      setTempPreset(null);
      setEditingPresetId(preset.id);
    } catch (err) {
      addLogEntry(`Error saving preset: ${err instanceof Error ? err.message : 'Unknown'}`);
    }
  };

  const handleDelete = async () => {
    if (!editingPreset) return;
    
    if (confirm(`Delete preset "${editingPreset.name}"?`)) {
      try {
        await apiDeletePreset(editingPreset.id);
        storeDeletePreset(editingPreset.id);
        addLogEntry(`Deleted preset: ${editingPreset.name}`);
        setEditingPresetId(null);
      } catch (err) {
        addLogEntry(`Error deleting preset: ${err instanceof Error ? err.message : 'Unknown'}`);
      }
    }
  };

  const handleCancel = () => {
    setIsCreatingNew(false);
    setTempPreset(null);
    setEditingPresetId(null);
  };

  const handlePreview = async (preset: Preset) => {
    if (selectedNodeIds.size === 0) {
      addLogEntry('No nodes selected for preview');
      return;
    }

    try {
      addLogEntry(`Previewing ${preset.name}...`);
      await applyPattern({
        node_ids: Array.from(selectedNodeIds),
        config: preset.config,
      });
      addLogEntry('Preview applied');
    } catch (err) {
      addLogEntry(`Preview error: ${err instanceof Error ? err.message : 'Unknown'}`);
    }
  };

  const currentPreset = isCreatingNew ? tempPreset : editingPreset;

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
