// Purpose: snake example with grid layers, fixed-rate loop, and arrow key tokens.
// Expected output: 40-column snapshot containing "snake", body cells, head, and food.
// Assumptions: run with host-shim.js and js/lib/*.js preloaded.
// Required globals: print, millis, gc.

var rt = Pico.createRuntime({ cols: 40, rows: 20, seed: 4 });
var OS = rt.OS;
OS.reset();
var game = OS.app("snake");
var st = game.state({ score: 0, status: "playing" });

var board = game.panel("board").frame("rounded").title(" snake ")
  .titleRight(function () { return "score " + Pico.pad(st.score, 3, " ") + " " + st.status; });
board.grid().at(0, 1).size(19, 9).cell("· ")
  .layer("body", function () { return OS.snake.cells; }, "█")
  .layer("head", function () { return [OS.snake.head]; }, "○")
  .layer("food", function () { return [OS.food]; }, "◆");

game.loop(8, function () {
  OS.step();
  if (OS.ate) st.score += 10;
  if (OS.dead) st.status = "dead";
});
game.key("↑↓←→", function (m, k) { OS.turn(k); });
game.statusbar("arrows move · deterministic demo");
game.mount();

rt.runFrame(130);
rt.sendKey("↓");
rt.runFrame(130);
print(rt.renderText());
