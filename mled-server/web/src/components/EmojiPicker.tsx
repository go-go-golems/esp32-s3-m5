import { useState, useCallback, useMemo } from 'preact/hooks';

// Common emojis organized by category with searchable keywords
const EMOJI_DATA: { emoji: string; keywords: string[] }[] = [
  // Lighting/Effects
  { emoji: '✨', keywords: ['sparkles', 'magic', 'shine', 'glitter', 'star'] },
  { emoji: '💡', keywords: ['bulb', 'light', 'idea', 'lamp'] },
  { emoji: '🔆', keywords: ['brightness', 'high', 'bright', 'sun'] },
  { emoji: '🔅', keywords: ['dim', 'low', 'brightness'] },
  { emoji: '⚡', keywords: ['lightning', 'bolt', 'flash', 'power', 'electric'] },
  { emoji: '🌟', keywords: ['star', 'glow', 'bright', 'shine'] },
  { emoji: '💫', keywords: ['dizzy', 'star', 'sparkle', 'magic'] },
  { emoji: '🌈', keywords: ['rainbow', 'colors', 'spectrum', 'pride'] },
  { emoji: '🎨', keywords: ['palette', 'art', 'colors', 'paint'] },
  { emoji: '🎭', keywords: ['theater', 'drama', 'masks', 'performance'] },
  
  // Colors
  { emoji: '🔴', keywords: ['red', 'circle', 'color'] },
  { emoji: '🟠', keywords: ['orange', 'circle', 'color'] },
  { emoji: '🟡', keywords: ['yellow', 'circle', 'color'] },
  { emoji: '🟢', keywords: ['green', 'circle', 'color'] },
  { emoji: '🔵', keywords: ['blue', 'circle', 'color'] },
  { emoji: '🟣', keywords: ['purple', 'circle', 'color'] },
  { emoji: '⚪', keywords: ['white', 'circle', 'color'] },
  { emoji: '⚫', keywords: ['black', 'circle', 'color', 'off'] },
  { emoji: '🟤', keywords: ['brown', 'circle', 'color'] },
  
  // Nature/Weather
  { emoji: '🔥', keywords: ['fire', 'hot', 'flame', 'warm', 'orange'] },
  { emoji: '❄️', keywords: ['snow', 'cold', 'ice', 'freeze', 'winter', 'blue'] },
  { emoji: '☀️', keywords: ['sun', 'sunny', 'bright', 'day', 'yellow'] },
  { emoji: '🌙', keywords: ['moon', 'night', 'crescent', 'dark'] },
  { emoji: '⭐', keywords: ['star', 'night', 'bright', 'yellow'] },
  { emoji: '🌊', keywords: ['wave', 'ocean', 'water', 'blue', 'sea'] },
  { emoji: '🌸', keywords: ['flower', 'cherry', 'blossom', 'pink', 'spring'] },
  { emoji: '🌺', keywords: ['hibiscus', 'flower', 'tropical', 'red'] },
  { emoji: '🌻', keywords: ['sunflower', 'flower', 'yellow', 'sun'] },
  { emoji: '🍀', keywords: ['clover', 'lucky', 'green', 'leaf'] },
  { emoji: '🌿', keywords: ['herb', 'plant', 'green', 'leaf', 'nature'] },
  
  // Mood/Feeling
  { emoji: '💙', keywords: ['heart', 'blue', 'love', 'calm'] },
  { emoji: '💚', keywords: ['heart', 'green', 'love', 'nature'] },
  { emoji: '💛', keywords: ['heart', 'yellow', 'love', 'warm'] },
  { emoji: '🧡', keywords: ['heart', 'orange', 'love', 'warm'] },
  { emoji: '💜', keywords: ['heart', 'purple', 'love'] },
  { emoji: '🤍', keywords: ['heart', 'white', 'love', 'pure'] },
  { emoji: '🖤', keywords: ['heart', 'black', 'love', 'dark'] },
  { emoji: '❤️', keywords: ['heart', 'red', 'love'] },
  { emoji: '💗', keywords: ['heart', 'growing', 'pink', 'love'] },
  { emoji: '😴', keywords: ['sleep', 'sleepy', 'tired', 'zzz', 'night'] },
  { emoji: '😎', keywords: ['cool', 'sunglasses', 'chill', 'relax'] },
  { emoji: '🥳', keywords: ['party', 'celebrate', 'fun', 'happy'] },
  { emoji: '🎉', keywords: ['party', 'celebrate', 'confetti', 'tada'] },
  
  // Objects
  { emoji: '🎵', keywords: ['music', 'note', 'song', 'sound'] },
  { emoji: '🎶', keywords: ['music', 'notes', 'song', 'melody'] },
  { emoji: '🔔', keywords: ['bell', 'notification', 'alert', 'ring'] },
  { emoji: '💎', keywords: ['diamond', 'gem', 'jewel', 'sparkle'] },
  { emoji: '🪩', keywords: ['disco', 'ball', 'party', 'dance', 'mirror'] },
  { emoji: '🕯️', keywords: ['candle', 'light', 'flame', 'warm'] },
  { emoji: '🏮', keywords: ['lantern', 'light', 'asian', 'red'] },
  { emoji: '🔦', keywords: ['flashlight', 'torch', 'light', 'beam'] },
  { emoji: '💠', keywords: ['diamond', 'blue', 'shape'] },
  { emoji: '🔮', keywords: ['crystal', 'ball', 'magic', 'purple', 'fortune'] },
  
  // Patterns/Shapes
  { emoji: '🌀', keywords: ['spiral', 'cyclone', 'swirl', 'dizzy'] },
  { emoji: '♾️', keywords: ['infinity', 'loop', 'forever', 'endless'] },
  { emoji: '〰️', keywords: ['wave', 'dash', 'wavy'] },
  { emoji: '➿', keywords: ['loop', 'curl', 'double'] },
  { emoji: '🔁', keywords: ['repeat', 'loop', 'cycle', 'again'] },
  { emoji: '🔄', keywords: ['refresh', 'arrows', 'sync', 'reload'] },
  
  // Power/Control
  { emoji: '⏸️', keywords: ['pause', 'stop', 'hold'] },
  { emoji: '▶️', keywords: ['play', 'start', 'go', 'forward'] },
  { emoji: '⏹️', keywords: ['stop', 'end', 'halt'] },
  { emoji: '⬛', keywords: ['black', 'square', 'off', 'stop'] },
  { emoji: '⬜', keywords: ['white', 'square', 'on', 'blank'] },
  { emoji: '🔲', keywords: ['button', 'black', 'square'] },
  { emoji: '🔳', keywords: ['button', 'white', 'square'] },
  { emoji: '🔘', keywords: ['radio', 'button', 'option', 'select'] },
];

interface EmojiPickerProps {
  value: string;
  onChange: (emoji: string) => void;
}

export function EmojiPicker({ value, onChange }: EmojiPickerProps) {
  const [isOpen, setIsOpen] = useState(false);
  const [search, setSearch] = useState('');

  const filteredEmojis = useMemo(() => {
    if (!search.trim()) return EMOJI_DATA;
    const searchLower = search.toLowerCase();
    return EMOJI_DATA.filter((item) =>
      item.keywords.some((kw) => kw.includes(searchLower)) ||
      item.emoji === search
    );
  }, [search]);

  const handleSelect = useCallback((emoji: string) => {
    onChange(emoji);
    setIsOpen(false);
    setSearch('');
  }, [onChange]);

  const handleToggle = useCallback(() => {
    setIsOpen((prev) => !prev);
    if (!isOpen) setSearch('');
  }, [isOpen]);

  const handleSearchChange = useCallback((e: Event) => {
    setSearch((e.target as HTMLInputElement).value);
  }, []);

  const handleKeyDown = useCallback((e: KeyboardEvent) => {
    if (e.key === 'Escape') {
      setIsOpen(false);
      setSearch('');
    }
  }, []);

  return (
    <div class="emoji-picker-container" onKeyDown={handleKeyDown}>
      <div class="emoji-picker-trigger">
        <button
          type="button"
          class="emoji-picker-btn"
          onClick={handleToggle}
          title="Select emoji"
        >
          <span class="emoji-preview">{value || '✨'}</span>
          <span class="emoji-arrow">{isOpen ? '▲' : '▼'}</span>
        </button>
      </div>
      
      {isOpen && (
        <div class="emoji-picker-dropdown">
          <div class="emoji-search-wrapper">
            <input
              type="text"
              class="form-control emoji-search"
              placeholder="Search: rainbow, fire, calm..."
              value={search}
              onInput={handleSearchChange}
              autoFocus
            />
          </div>
          <div class="emoji-grid">
            {filteredEmojis.length === 0 ? (
              <div class="emoji-no-results">No emojis found</div>
            ) : (
              filteredEmojis.map((item) => (
                <button
                  key={item.emoji}
                  type="button"
                  class={`emoji-option ${value === item.emoji ? 'selected' : ''}`}
                  onClick={() => handleSelect(item.emoji)}
                  title={item.keywords.join(', ')}
                >
                  {item.emoji}
                </button>
              ))
            )}
          </div>
        </div>
      )}
    </div>
  );
}
