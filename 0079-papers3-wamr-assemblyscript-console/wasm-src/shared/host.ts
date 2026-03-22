@external("host", "host_log_i32")
export declare function logI32(tag: i32, value: i32): void;

@external("host", "host_delay_ms")
export declare function delayMs(ms: i32): void;

@external("host", "host_screen_clear")
export declare function screenClear(color: i32): void;

@external("host", "host_draw_rect")
export declare function drawRect(x: i32, y: i32, w: i32, h: i32, color: i32): void;

@external("host", "host_fill_rect")
export declare function fillRect(x: i32, y: i32, w: i32, h: i32, color: i32): void;

@external("host", "host_present")
export declare function present(mode: i32): void;

export const DISPLAY_WIDTH: i32 = 960;
export const DISPLAY_HEIGHT: i32 = 540;

export const BLACK: i32 = 0x000000;
export const WHITE: i32 = 0xFFFFFF;
export const MID_GRAY: i32 = 0x8C8C8C;
export const LIGHT_GRAY: i32 = 0xD9D9D9;

export function rgb(r: i32, g: i32, b: i32): i32 {
  return ((r & 0xFF) << 16) | ((g & 0xFF) << 8) | (b & 0xFF);
}

export function clamp(value: i32, minValue: i32, maxValue: i32): i32 {
  if (value < minValue) {
    return minValue;
  }
  if (value > maxValue) {
    return maxValue;
  }
  return value;
}
