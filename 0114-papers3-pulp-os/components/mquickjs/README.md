# mquickjs (vendored engine copy for 0114-papers3-pulp-os)

MicroQuickJS by Fabrice Bellard and Charlie Gordon, MIT license (see the
headers of the source files). Copied from
`0112-papers3-reader-primitives/components/mquickjs`, which was vendored
from the in-repo import `imports/esp32-mqjs-repl` (upstream reference:
bellard/mquickjs @ 84d793e0).

This copy is LOCAL TO THIS FIRMWARE by design: `mquickjs_atom.h` is
generated from THIS firmware's stdlib (`tools/js/pulp_stdlib.c`, the v2
builder API with native Widget/Page classes) and is incompatible with any
other stdlib. Never share an engine component between firmwares with
different stdlibs; bytecode images are atom-coupled too.

Regeneration: `tools/js/gen_pulp_stdlib.sh` (device headers), then
`tools/js/build_bytecode_apps.sh` (host compiler + bytecode apps).
