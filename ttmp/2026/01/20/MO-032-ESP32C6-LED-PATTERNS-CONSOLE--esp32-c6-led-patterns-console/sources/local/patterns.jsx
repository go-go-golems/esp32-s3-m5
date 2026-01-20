import { useState, useEffect, useRef, useCallback } from 'react';

const LED_COUNT = 50;

// Color utility functions
const hslToRgb = (h, s, l) => {
  s /= 100;
  l /= 100;
  const k = n => (n + h / 30) % 12;
  const a = s * Math.min(l, 1 - l);
  const f = n => l - a * Math.max(-1, Math.min(k(n) - 3, Math.min(9 - k(n), 1)));
  return { r: Math.round(f(0) * 255), g: Math.round(f(8) * 255), b: Math.round(f(4) * 255) };
};

const rgbToString = (r, g, b, brightness = 1) =>
  `rgb(${Math.round(r * brightness)}, ${Math.round(g * brightness)}, ${Math.round(b * brightness)})`;

const hexToRgb = (hex) => {
  const result = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex);
  return result ? {
    r: parseInt(result[1], 16),
    g: parseInt(result[2], 16),
    b: parseInt(result[3], 16)
  } : { r: 255, g: 255, b: 255 };
};

// Pattern generators
const patterns = {
  solid: (leds, frame, { color1, brightness }) => {
    const rgb = hexToRgb(color1);
    return leds.map(() => rgbToString(rgb.r, rgb.g, rgb.b, brightness));
  },

  rainbow: (leds, frame, { speed, brightness }) => {
    return leds.map((_, i) => {
      const hue = ((i * 360 / LED_COUNT) + frame * speed) % 360;
      const rgb = hslToRgb(hue, 100, 50);
      return rgbToString(rgb.r, rgb.g, rgb.b, brightness);
    });
  },

  rainbowWave: (leds, frame, { speed, brightness }) => {
    return leds.map((_, i) => {
      const hue = ((i * 720 / LED_COUNT) + frame * speed * 2) % 360;
      const rgb = hslToRgb(hue, 100, 50);
      return rgbToString(rgb.r, rgb.g, rgb.b, brightness);
    });
  },

  chase: (leds, frame, { color1, color2, speed, brightness }) => {
    const rgb1 = hexToRgb(color1);
    const rgb2 = hexToRgb(color2);
    const chaseLength = 5;
    return leds.map((_, i) => {
      const pos = Math.floor(frame * speed / 3) % LED_COUNT;
      const dist = (i - pos + LED_COUNT) % LED_COUNT;
      if (dist < chaseLength) {
        const fade = 1 - (dist / chaseLength);
        return rgbToString(rgb1.r, rgb1.g, rgb1.b, brightness * fade);
      }
      return rgbToString(rgb2.r * 0.1, rgb2.g * 0.1, rgb2.b * 0.1, brightness);
    });
  },

  dualChase: (leds, frame, { color1, color2, speed, brightness }) => {
    const rgb1 = hexToRgb(color1);
    const rgb2 = hexToRgb(color2);
    const chaseLength = 5;
    return leds.map((_, i) => {
      const pos1 = Math.floor(frame * speed / 3) % LED_COUNT;
      const pos2 = (LED_COUNT - 1 - Math.floor(frame * speed / 3) % LED_COUNT);
      const dist1 = (i - pos1 + LED_COUNT) % LED_COUNT;
      const dist2 = (i - pos2 + LED_COUNT) % LED_COUNT;
      if (dist1 < chaseLength) {
        const fade = 1 - (dist1 / chaseLength);
        return rgbToString(rgb1.r, rgb1.g, rgb1.b, brightness * fade);
      }
      if (dist2 < chaseLength) {
        const fade = 1 - (dist2 / chaseLength);
        return rgbToString(rgb2.r, rgb2.g, rgb2.b, brightness * fade);
      }
      return 'rgb(5, 5, 5)';
    });
  },

  breathing: (leds, frame, { color1, speed, brightness }) => {
    const rgb = hexToRgb(color1);
    const breath = (Math.sin(frame * speed * 0.05) + 1) / 2;
    return leds.map(() => rgbToString(rgb.r, rgb.g, rgb.b, brightness * breath * 0.9 + 0.1));
  },

  fire: (leds, frame, { speed, brightness }) => {
    return leds.map((_, i) => {
      const flicker = Math.random() * 0.4 + 0.6;
      const heat = Math.sin(i * 0.3 + frame * speed * 0.1) * 0.3 + 0.7;
      const r = 255;
      const g = Math.floor(80 * heat * flicker);
      const b = Math.floor(10 * flicker);
      return rgbToString(r, g, b, brightness * flicker);
    });
  },

  sparkle: (leds, frame, { color1, speed, brightness }) => {
    const rgb = hexToRgb(color1);
    return leds.map(() => {
      const sparkle = Math.random() > 0.92 ? 1 : 0.05;
      return rgbToString(rgb.r, rgb.g, rgb.b, brightness * sparkle);
    });
  },

  colorSparkle: (leds, frame, { brightness }) => {
    return leds.map(() => {
      if (Math.random() > 0.92) {
        const hue = Math.random() * 360;
        const rgb = hslToRgb(hue, 100, 50);
        return rgbToString(rgb.r, rgb.g, rgb.b, brightness);
      }
      return 'rgb(5, 5, 5)';
    });
  },

  meteor: (leds, frame, { color1, speed, brightness }) => {
    const rgb = hexToRgb(color1);
    const meteorLength = 10;
    const pos = Math.floor(frame * speed / 2) % (LED_COUNT + meteorLength);
    return leds.map((_, i) => {
      const dist = pos - i;
      if (dist >= 0 && dist < meteorLength) {
        const fade = 1 - (dist / meteorLength);
        const flicker = Math.random() * 0.3 + 0.7;
        return rgbToString(rgb.r, rgb.g, rgb.b, brightness * fade * flicker);
      }
      // Random decay for tail
      return Math.random() > 0.9 ? rgbToString(rgb.r * 0.2, rgb.g * 0.2, rgb.b * 0.2, brightness * 0.3) : 'rgb(2, 2, 2)';
    });
  },

  wave: (leds, frame, { color1, color2, speed, brightness }) => {
    const rgb1 = hexToRgb(color1);
    const rgb2 = hexToRgb(color2);
    return leds.map((_, i) => {
      const wave = (Math.sin((i * 0.3) - frame * speed * 0.1) + 1) / 2;
      const r = Math.round(rgb1.r * wave + rgb2.r * (1 - wave));
      const g = Math.round(rgb1.g * wave + rgb2.g * (1 - wave));
      const b = Math.round(rgb1.b * wave + rgb2.b * (1 - wave));
      return rgbToString(r, g, b, brightness);
    });
  },

  gradient: (leds, frame, { color1, color2, speed, brightness }) => {
    const rgb1 = hexToRgb(color1);
    const rgb2 = hexToRgb(color2);
    const offset = (frame * speed * 0.02) % 1;
    return leds.map((_, i) => {
      let t = ((i / LED_COUNT) + offset) % 1;
      if (t > 0.5) t = 1 - t;
      t *= 2;
      const r = Math.round(rgb1.r * (1 - t) + rgb2.r * t);
      const g = Math.round(rgb1.g * (1 - t) + rgb2.g * t);
      const b = Math.round(rgb1.b * (1 - t) + rgb2.b * t);
      return rgbToString(r, g, b, brightness);
    });
  },

  police: (leds, frame, { speed, brightness }) => {
    const cycle = Math.floor(frame * speed / 10) % 4;
    return leds.map((_, i) => {
      const half = i < LED_COUNT / 2;
      if (cycle < 2) {
        return half ? rgbToString(255, 0, 0, brightness) : rgbToString(0, 0, 255, brightness * 0.1);
      } else {
        return half ? rgbToString(255, 0, 0, brightness * 0.1) : rgbToString(0, 0, 255, brightness);
      }
    });
  },

  strobe: (leds, frame, { color1, speed, brightness }) => {
    const rgb = hexToRgb(color1);
    const on = Math.floor(frame * speed / 5) % 2 === 0;
    return leds.map(() => on ? rgbToString(rgb.r, rgb.g, rgb.b, brightness) : 'rgb(0, 0, 0)');
  },

  comet: (leds, frame, { speed, brightness }) => {
    const tailLength = 15;
    const pos = Math.floor(frame * speed / 2) % (LED_COUNT + tailLength);
    return leds.map((_, i) => {
      const dist = pos - i;
      if (dist >= 0 && dist < tailLength) {
        const fade = 1 - (dist / tailLength);
        const hue = (360 - dist * 20 + frame * 2) % 360;
        const rgb = hslToRgb(hue, 100, 50);
        return rgbToString(rgb.r, rgb.g, rgb.b, brightness * fade);
      }
      return 'rgb(0, 0, 0)';
    });
  },

  bounce: (leds, frame, { color1, speed, brightness }) => {
    const rgb = hexToRgb(color1);
    const ballSize = 5;
    const period = LED_COUNT - ballSize;
    const rawPos = Math.floor(frame * speed / 3) % (period * 2);
    const pos = rawPos < period ? rawPos : period * 2 - rawPos;
    return leds.map((_, i) => {
      if (i >= pos && i < pos + ballSize) {
        return rgbToString(rgb.r, rgb.g, rgb.b, brightness);
      }
      return 'rgb(3, 3, 3)';
    });
  },

  twinkle: (leds, frame, { color1, brightness }) => {
    const rgb = hexToRgb(color1);
    return leds.map((_, i) => {
      const phase = (i * 1.7 + frame * 0.1) % (Math.PI * 2);
      const twinkle = (Math.sin(phase) + 1) / 2;
      return rgbToString(rgb.r, rgb.g, rgb.b, brightness * twinkle * 0.8 + 0.1);
    });
  },

  christmas: (leds, frame, { speed, brightness }) => {
    const offset = Math.floor(frame * speed / 10) % 2;
    return leds.map((_, i) => {
      const isRed = (i + offset) % 2 === 0;
      return isRed ? rgbToString(255, 0, 0, brightness) : rgbToString(0, 255, 0, brightness);
    });
  },

  ocean: (leds, frame, { speed, brightness }) => {
    return leds.map((_, i) => {
      const wave1 = Math.sin(i * 0.2 + frame * speed * 0.05) * 0.5 + 0.5;
      const wave2 = Math.sin(i * 0.1 - frame * speed * 0.03) * 0.5 + 0.5;
      const combined = (wave1 + wave2) / 2;
      const r = Math.floor(0);
      const g = Math.floor(100 + combined * 100);
      const b = Math.floor(150 + combined * 105);
      return rgbToString(r, g, b, brightness);
    });
  },

  plasma: (leds, frame, { speed, brightness }) => {
    return leds.map((_, i) => {
      const v1 = Math.sin(i * 0.3 + frame * speed * 0.02);
      const v2 = Math.sin(i * 0.1 + frame * speed * 0.03);
      const v3 = Math.sin((i * 0.2 + frame * speed * 0.01) * 0.5);
      const v = (v1 + v2 + v3) / 3;
      const hue = ((v + 1) * 180 + frame * speed) % 360;
      const rgb = hslToRgb(hue, 100, 50);
      return rgbToString(rgb.r, rgb.g, rgb.b, brightness);
    });
  },

  segments: (leds, frame, { speed, brightness }) => {
    const segmentSize = 10;
    const offset = Math.floor(frame * speed / 5) % 5;
    const colors = [
      { r: 255, g: 0, b: 0 },
      { r: 255, g: 165, b: 0 },
      { r: 255, g: 255, b: 0 },
      { r: 0, g: 255, b: 0 },
      { r: 0, g: 0, b: 255 },
    ];
    return leds.map((_, i) => {
      const segment = Math.floor(i / segmentSize);
      const colorIndex = (segment + offset) % colors.length;
      const c = colors[colorIndex];
      return rgbToString(c.r, c.g, c.b, brightness);
    });
  },
};

const presets = [
  { name: '🌈 Rainbow', pattern: 'rainbow', color1: '#ff0000', color2: '#0000ff', speed: 3 },
  { name: '🌊 Rainbow Wave', pattern: 'rainbowWave', color1: '#ff0000', color2: '#0000ff', speed: 2 },
  { name: '🔴 Solid Red', pattern: 'solid', color1: '#ff0000', color2: '#000000', speed: 1 },
  { name: '🟢 Solid Green', pattern: 'solid', color1: '#00ff00', color2: '#000000', speed: 1 },
  { name: '🔵 Solid Blue', pattern: 'solid', color1: '#0000ff', color2: '#000000', speed: 1 },
  { name: '⬜ Warm White', pattern: 'solid', color1: '#ffcc88', color2: '#000000', speed: 1 },
  { name: '🏃 Chase', pattern: 'chase', color1: '#00ffff', color2: '#000044', speed: 5 },
  { name: '🏃‍♂️ Dual Chase', pattern: 'dualChase', color1: '#ff0000', color2: '#0000ff', speed: 5 },
  { name: '💨 Breathing', pattern: 'breathing', color1: '#ff00ff', color2: '#000000', speed: 3 },
  { name: '🔥 Fire', pattern: 'fire', color1: '#ff4400', color2: '#000000', speed: 5 },
  { name: '✨ Sparkle', pattern: 'sparkle', color1: '#ffffff', color2: '#000000', speed: 1 },
  { name: '🎆 Color Sparkle', pattern: 'colorSparkle', color1: '#ffffff', color2: '#000000', speed: 1 },
  { name: '☄️ Meteor', pattern: 'meteor', color1: '#ffffff', color2: '#000000', speed: 4 },
  { name: '🌊 Wave', pattern: 'wave', color1: '#ff0088', color2: '#0088ff', speed: 3 },
  { name: '🎨 Gradient', pattern: 'gradient', color1: '#ff0000', color2: '#00ff00', speed: 2 },
  { name: '🚔 Police', pattern: 'police', color1: '#ff0000', color2: '#0000ff', speed: 6 },
  { name: '⚡ Strobe', pattern: 'strobe', color1: '#ffffff', color2: '#000000', speed: 8 },
  { name: '💫 Comet', pattern: 'comet', color1: '#ff0000', color2: '#000000', speed: 4 },
  { name: '🏐 Bounce', pattern: 'bounce', color1: '#ffff00', color2: '#000000', speed: 4 },
  { name: '💎 Twinkle', pattern: 'twinkle', color1: '#aaddff', color2: '#000000', speed: 1 },
  { name: '🎄 Christmas', pattern: 'christmas', color1: '#ff0000', color2: '#00ff00', speed: 2 },
  { name: '🌊 Ocean', pattern: 'ocean', color1: '#0066ff', color2: '#00ffff', speed: 3 },
  { name: '🔮 Plasma', pattern: 'plasma', color1: '#ff00ff', color2: '#00ffff', speed: 3 },
  { name: '🧱 Segments', pattern: 'segments', color1: '#ff0000', color2: '#0000ff', speed: 3 },
];

export default function LEDSimulator() {
  const [leds, setLeds] = useState(Array(LED_COUNT).fill('rgb(0, 0, 0)'));
  const [pattern, setPattern] = useState('rainbow');
  const [color1, setColor1] = useState('#ff0000');
  const [color2, setColor2] = useState('#0000ff');
  const [speed, setSpeed] = useState(3);
  const [brightness, setBrightness] = useState(1);
  const [isPlaying, setIsPlaying] = useState(true);
  const [layout, setLayout] = useState('line');
  const frameRef = useRef(0);
  const animationRef = useRef(null);

  const updateLeds = useCallback(() => {
    if (!isPlaying) return;

    const patternFn = patterns[pattern];
    if (patternFn) {
      const newLeds = patternFn(leds, frameRef.current, { color1, color2, speed, brightness });
      setLeds(newLeds);
    }
    frameRef.current += 1;
    animationRef.current = requestAnimationFrame(updateLeds);
  }, [pattern, color1, color2, speed, brightness, isPlaying]);

  useEffect(() => {
    if (isPlaying) {
      animationRef.current = requestAnimationFrame(updateLeds);
    }
    return () => {
      if (animationRef.current) {
        cancelAnimationFrame(animationRef.current);
      }
    };
  }, [updateLeds, isPlaying]);

  const applyPreset = (preset) => {
    setPattern(preset.pattern);
    setColor1(preset.color1);
    setColor2(preset.color2);
    setSpeed(preset.speed);
  };

  const renderLEDs = () => {
    const ledElements = leds.map((color, i) => (
      <div
        key={i}
        className="rounded-full transition-all duration-75"
        style={{
          backgroundColor: color,
          boxShadow: `0 0 8px 2px ${color}, 0 0 16px 4px ${color}40`,
          width: layout === 'grid' ? '16px' : '12px',
          height: layout === 'grid' ? '16px' : '12px',
        }}
        title={`LED ${i + 1}`}
      />
    ));

    if (layout === 'line') {
      return (
        <div className="flex flex-wrap gap-1 justify-center p-4 bg-gray-900 rounded-lg">
          {ledElements}
        </div>
      );
    } else if (layout === 'circle') {
      return (
        <div className="relative w-72 h-72 mx-auto bg-gray-900 rounded-full">
          {leds.map((color, i) => {
            const angle = (i / LED_COUNT) * Math.PI * 2 - Math.PI / 2;
            const radius = 130;
            const x = Math.cos(angle) * radius + 144;
            const y = Math.sin(angle) * radius + 144;
            return (
              <div
                key={i}
                className="absolute rounded-full transition-all duration-75"
                style={{
                  backgroundColor: color,
                  boxShadow: `0 0 8px 2px ${color}, 0 0 16px 4px ${color}40`,
                  width: '10px',
                  height: '10px',
                  left: `${x}px`,
                  top: `${y}px`,
                  transform: 'translate(-50%, -50%)',
                }}
                title={`LED ${i + 1}`}
              />
            );
          })}
        </div>
      );
    } else if (layout === 'grid') {
      return (
        <div className="grid grid-cols-10 gap-1 p-4 bg-gray-900 rounded-lg w-fit mx-auto">
          {ledElements}
        </div>
      );
    } else if (layout === 'zigzag') {
      return (
        <div className="flex flex-col gap-1 p-4 bg-gray-900 rounded-lg">
          {[0, 1, 2, 3, 4].map((row) => (
            <div key={row} className={`flex gap-1 ${row % 2 === 1 ? 'flex-row-reverse' : ''}`}>
              {leds.slice(row * 10, row * 10 + 10).map((color, i) => (
                <div
                  key={row * 10 + i}
                  className="rounded-full transition-all duration-75"
                  style={{
                    backgroundColor: color,
                    boxShadow: `0 0 8px 2px ${color}, 0 0 16px 4px ${color}40`,
                    width: '16px',
                    height: '16px',
                  }}
                  title={`LED ${row * 10 + i + 1}`}
                />
              ))}
            </div>
          ))}
        </div>
      );
    }
  };

  return (
    <div className="min-h-screen bg-gray-950 text-white p-4">
      <div className="max-w-4xl mx-auto">
        <h1 className="text-2xl font-bold text-center mb-2">💡 WS2811 LED Simulator</h1>
        <p className="text-gray-400 text-center mb-4">50 RGB LEDs</p>

        {/* LED Display */}
        <div className="mb-6">
          {renderLEDs()}
        </div>

        {/* Layout Selector */}
        <div className="flex justify-center gap-2 mb-6">
          {[
            { id: 'line', label: '➖ Line' },
            { id: 'grid', label: '⊞ Grid' },
            { id: 'circle', label: '⭕ Circle' },
            { id: 'zigzag', label: '〰️ Zigzag' },
          ].map((l) => (
            <button
              key={l.id}
              onClick={() => setLayout(l.id)}
              className={`px-3 py-1 rounded text-sm ${layout === l.id ? 'bg-blue-600' : 'bg-gray-700 hover:bg-gray-600'}`}
            >
              {l.label}
            </button>
          ))}
        </div>

        {/* Controls */}
        <div className="bg-gray-800 rounded-lg p-4 mb-4">
          <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
            {/* Pattern Selector */}
            <div>
              <label className="block text-sm text-gray-400 mb-1">Pattern</label>
              <select
                value={pattern}
                onChange={(e) => setPattern(e.target.value)}
                className="w-full bg-gray-700 rounded px-3 py-2 text-white"
              >
                {Object.keys(patterns).map((p) => (
                  <option key={p} value={p}>
                    {p.charAt(0).toUpperCase() + p.slice(1).replace(/([A-Z])/g, ' $1')}
                  </option>
                ))}
              </select>
            </div>

            {/* Play/Pause */}
            <div className="flex items-end gap-2">
              <button
                onClick={() => setIsPlaying(!isPlaying)}
                className={`px-4 py-2 rounded font-medium ${isPlaying ? 'bg-red-600 hover:bg-red-700' : 'bg-green-600 hover:bg-green-700'}`}
              >
                {isPlaying ? '⏸️ Pause' : '▶️ Play'}
              </button>
              <button
                onClick={() => { frameRef.current = 0; }}
                className="px-4 py-2 rounded bg-gray-600 hover:bg-gray-500"
              >
                🔄 Reset
              </button>
            </div>

            {/* Color 1 */}
            <div>
              <label className="block text-sm text-gray-400 mb-1">Color 1</label>
              <div className="flex gap-2">
                <input
                  type="color"
                  value={color1}
                  onChange={(e) => setColor1(e.target.value)}
                  className="w-12 h-10 rounded cursor-pointer bg-transparent"
                />
                <input
                  type="text"
                  value={color1}
                  onChange={(e) => setColor1(e.target.value)}
                  className="flex-1 bg-gray-700 rounded px-3 py-2 text-white font-mono text-sm"
                />
              </div>
            </div>

            {/* Color 2 */}
            <div>
              <label className="block text-sm text-gray-400 mb-1">Color 2</label>
              <div className="flex gap-2">
                <input
                  type="color"
                  value={color2}
                  onChange={(e) => setColor2(e.target.value)}
                  className="w-12 h-10 rounded cursor-pointer bg-transparent"
                />
                <input
                  type="text"
                  value={color2}
                  onChange={(e) => setColor2(e.target.value)}
                  className="flex-1 bg-gray-700 rounded px-3 py-2 text-white font-mono text-sm"
                />
              </div>
            </div>

            {/* Speed */}
            <div>
              <label className="block text-sm text-gray-400 mb-1">Speed: {speed}</label>
              <input
                type="range"
                min="1"
                max="10"
                value={speed}
                onChange={(e) => setSpeed(Number(e.target.value))}
                className="w-full"
              />
            </div>

            {/* Brightness */}
            <div>
              <label className="block text-sm text-gray-400 mb-1">Brightness: {Math.round(brightness * 100)}%</label>
              <input
                type="range"
                min="0.1"
                max="1"
                step="0.05"
                value={brightness}
                onChange={(e) => setBrightness(Number(e.target.value))}
                className="w-full"
              />
            </div>
          </div>
        </div>

        {/* Presets */}
        <div className="bg-gray-800 rounded-lg p-4">
          <h2 className="text-lg font-semibold mb-3">🎨 Presets</h2>
          <div className="grid grid-cols-2 sm:grid-cols-3 md:grid-cols-4 gap-2">
            {presets.map((preset, i) => (
              <button
                key={i}
                onClick={() => applyPreset(preset)}
                className={`px-3 py-2 rounded text-sm text-left transition-colors ${pattern === preset.pattern && color1 === preset.color1
                    ? 'bg-blue-600 ring-2 ring-blue-400'
                    : 'bg-gray-700 hover:bg-gray-600'
                  }`}
              >
                {preset.name}
              </button>
            ))}
          </div>
        </div>

        {/* Info */}
        <div className="mt-4 text-center text-gray-500 text-sm">
          <p>Simulating WS2811 addressable RGB LED strip • {LED_COUNT} LEDs • Frame: {frameRef.current}</p>
        </div>
      </div>
    </div>
  );
}
