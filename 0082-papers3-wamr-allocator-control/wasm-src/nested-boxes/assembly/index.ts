import {
  BLACK,
  DISPLAY_HEIGHT,
  DISPLAY_WIDTH,
  WHITE,
  drawRect,
  logI32,
  present,
  rgb,
  screenClear
} from "../../shared/host";

export function run(): i32 {
  screenClear(WHITE);

  let inset: i32 = 24;
  for (let i: i32 = 0; i < 9; i++) {
    drawRect(
      inset,
      inset,
      DISPLAY_WIDTH - inset * 2,
      DISPLAY_HEIGHT - inset * 2,
      i == 8 ? BLACK : rgb(24 + i * 18, 24 + i * 18, 24 + i * 18)
    );
    inset += 24;
  }

  present(1);
  logI32(2, inset);
  return 0;
}
