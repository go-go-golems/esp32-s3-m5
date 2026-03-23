import {
  BLACK,
  DISPLAY_HEIGHT,
  DISPLAY_WIDTH,
  WHITE,
  clamp,
  drawRect,
  fillRect,
  logI32,
  present,
  rgb,
  screenClear
} from "../../shared/host";

export function run(): i32 {
  screenClear(WHITE);

  const centerX: i32 = DISPLAY_WIDTH / 2;
  const centerY: i32 = DISPLAY_HEIGHT / 2;
  const radius: i32 = 180;

  drawRect(centerX - radius, centerY - radius, radius * 2, radius * 2, BLACK);
  drawRect(centerX - radius + 24, centerY - radius + 24, (radius - 24) * 2, (radius - 24) * 2, BLACK);

  for (let band: i32 = 0; band < 8; band++) {
    const bandX: i32 = centerX - radius + 16 + band * 42;
    const bandHeight: i32 = clamp(40 + band * 18, 40, radius * 2 - 32);
    fillRect(bandX, centerY - bandHeight / 2, 18, bandHeight, rgb(30 + band * 20, 30 + band * 20, 30 + band * 20));
  }

  fillRect(centerX - 4, centerY - radius + 8, 8, radius * 2 - 16, BLACK);
  fillRect(centerX - radius + 8, centerY - 4, radius * 2 - 16, 8, BLACK);

  for (let i: i32 = 0; i < 6; i++) {
    const step: i32 = 24 + i * 26;
    fillRect(centerX + step, centerY - step, 12, 12, rgb(40 + i * 25, 40 + i * 25, 40 + i * 25));
  }

  present(1);
  logI32(5, radius);
  return 0;
}
