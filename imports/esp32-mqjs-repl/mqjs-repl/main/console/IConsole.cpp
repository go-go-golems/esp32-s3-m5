#include "console/IConsole.h"

#include <string.h>

void IConsole::WriteString(const char* s) {
  if (!s) {
    return;
  }
  Write(reinterpret_cast<const uint8_t*>(s), static_cast<int>(strlen(s)));
}

