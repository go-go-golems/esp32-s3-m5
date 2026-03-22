import {
  BLACK,
  DISPLAY_HEIGHT,
  DISPLAY_WIDTH,
  MID_GRAY,
  WHITE,
  drawRect,
  fillRect,
  logI32,
  present,
  screenClear
} from "../../shared/host";

export function run(): i32 {
  screenClear(WHITE);

  drawRect(16, 16, DISPLAY_WIDTH - 32, DISPLAY_HEIGHT - 32, BLACK);
  drawRect(28, 28, DISPLAY_WIDTH - 56, DISPLAY_HEIGHT - 56, MID_GRAY);

  fillRect(56, 72, 260, 92, BLACK);
  fillRect(70, 86, 232, 64, WHITE);
  fillRect(DISPLAY_WIDTH - 320, DISPLAY_HEIGHT - 164, 248, 76, MID_GRAY);
  drawRect(DISPLAY_WIDTH - 320, DISPLAY_HEIGHT - 164, 248, 76, BLACK);

  present(1);
  logI32(1, 79);
  return 0;
}
