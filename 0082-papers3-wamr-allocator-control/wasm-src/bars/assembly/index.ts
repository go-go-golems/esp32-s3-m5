import {
  BLACK,
  DISPLAY_HEIGHT,
  LIGHT_GRAY,
  WHITE,
  drawRect,
  fillRect,
  logI32,
  present,
  screenClear
} from "../../shared/host";

export function run(): i32 {
  screenClear(WHITE);
  drawRect(40, 40, 880, DISPLAY_HEIGHT - 80, BLACK);

  const baseY: i32 = DISPLAY_HEIGHT - 88;
  for (let i: i32 = 0; i < 10; i++) {
    const barHeight: i32 = 48 + i * 28;
    const x: i32 = 72 + i * 80;
    fillRect(x, baseY - barHeight, 48, barHeight, LIGHT_GRAY);
    drawRect(x, baseY - barHeight, 48, barHeight, BLACK);
  }

  present(1);
  logI32(3, baseY);
  return 0;
}
