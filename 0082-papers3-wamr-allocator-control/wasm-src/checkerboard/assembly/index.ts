import {
  BLACK,
  LIGHT_GRAY,
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

  const squareSize: i32 = 72;
  for (let row: i32 = 0; row < 5; row++) {
    for (let col: i32 = 0; col < 10; col++) {
      const x: i32 = 96 + col * squareSize;
      const y: i32 = 84 + row * squareSize;
      const color: i32 = ((row + col) & 1) == 0 ? LIGHT_GRAY : MID_GRAY;
      fillRect(x, y, squareSize, squareSize, color);
      drawRect(x, y, squareSize, squareSize, BLACK);
    }
  }

  present(1);
  logI32(4, squareSize);
  return 0;
}
