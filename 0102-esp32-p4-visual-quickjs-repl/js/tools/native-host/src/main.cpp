#include "pico_native_api.hpp"

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <string>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

namespace {

struct RawTerminal {
  termios oldt{};
  bool active = false;
  RawTerminal() {
    if (tcgetattr(STDIN_FILENO, &oldt) == 0) {
      termios raw = oldt;
      raw.c_lflag &= (tcflag_t)~(ICANON | ECHO);
      raw.c_cc[VMIN] = 0;
      raw.c_cc[VTIME] = 0;
      if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) == 0) active = true;
    }
  }
  ~RawTerminal() { if (active) tcsetattr(STDIN_FILENO, TCSANOW, &oldt); }
};

bool g_stop = false;
void on_sigint(int) { g_stop = true; }

std::string dirname_of(const std::string &path) {
  size_t p = path.rfind('/');
  if (p == std::string::npos) return ".";
  return path.substr(0, p);
}

bool poll_key(std::string *tok) {
  fd_set rfds;
  FD_ZERO(&rfds);
  FD_SET(STDIN_FILENO, &rfds);
  timeval tv{0, 0};
  int ready = select(STDIN_FILENO + 1, &rfds, nullptr, nullptr, &tv);
  if (ready <= 0) return false;
  unsigned char ch = 0;
  if (read(STDIN_FILENO, &ch, 1) != 1) return false;
  if (ch == 3) { g_stop = true; return false; }
  if (ch == 27) {
    unsigned char seq[2] = {0, 0};
    if (read(STDIN_FILENO, &seq[0], 1) == 1 && read(STDIN_FILENO, &seq[1], 1) == 1 && seq[0] == '[') {
      if (seq[1] == 'A') *tok = "↑";
      else if (seq[1] == 'B') *tok = "↓";
      else if (seq[1] == 'C') *tok = "→";
      else if (seq[1] == 'D') *tok = "←";
      else *tok = "esc";
    } else *tok = "esc";
    return true;
  }
  if (ch == '\r' || ch == '\n') *tok = "⏎";
  else if (ch == 127 || ch == 8) *tok = "⌫";
  else *tok = std::string(1, (char)ch);
  return true;
}

void draw(const std::string &text, const std::string &name) {
  std::printf("\033[H\033[2J");
  std::printf("pico-native-host: %s  (arrows/type send keys, q quits host)\n", name.c_str());
  std::printf("%s\n", text.c_str());
  std::fflush(stdout);
}

}  // namespace

int main(int argc, char **argv) {
  std::signal(SIGINT, on_sigint);
  const char *env_js_dir = std::getenv("PICO_JS_DIR");
  std::string self = argc > 0 ? argv[0] : "picojs-host";
  std::string js_root = env_js_dir ? env_js_dir : dirname_of(dirname_of(dirname_of(self)));
  std::string example = argc > 1 ? argv[1] : "hello-native";
  std::string path = example.find('/') == std::string::npos
    ? js_root + "/examples-native/" + example + ".js"
    : example;

  auto *rt = pico_native::runtime_create(40, 30);
  std::string error;
  if (!pico_native::runtime_load_file(rt, path, &error)) {
    std::fprintf(stderr, "load failed: %s\n", error.c_str());
    pico_native::runtime_destroy(rt);
    return 1;
  }

  RawTerminal term;
  std::printf("\033[?25l");
  uint64_t last = pico_native::host_millis();
  while (!g_stop) {
    std::string tok;
    while (poll_key(&tok)) {
      if (tok == "q") { g_stop = true; break; }
      pico_native::runtime_send_key(rt, tok);
    }
    uint64_t now = pico_native::host_millis();
    int dt = (int)(now - last);
    if (dt < 0) dt = 0;
    last = now;
    pico_native::runtime_run_frame(rt, dt);
    draw(pico_native::runtime_render_text(rt), example);
    usleep(100000);
  }
  std::printf("\033[?25h\033[0m\n");
  pico_native::runtime_destroy(rt);
  return 0;
}
