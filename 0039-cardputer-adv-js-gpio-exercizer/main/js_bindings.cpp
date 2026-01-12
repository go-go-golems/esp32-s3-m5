#include "js_bindings.h"

#include "exercizer/ControlPlane.h"
#include "exercizer/ControlPlaneC.h"

void exercizer_js_bind(ControlPlane *ctrl) {
  if (!ctrl) {
    return;
  }
  exercizer_control_set_default(static_cast<void *>(ctrl));
}
