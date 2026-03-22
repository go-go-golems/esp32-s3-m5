(module
 (type $0 (func (param i32)))
 (type $1 (func (param i32 i32 i32 i32 i32)))
 (type $2 (func (param i32 i32)))
 (type $3 (func (result i32)))
 (import "host" "host_screen_clear" (func $shared/host/screenClear (param i32)))
 (import "host" "host_draw_rect" (func $shared/host/drawRect (param i32 i32 i32 i32 i32)))
 (import "host" "host_fill_rect" (func $shared/host/fillRect (param i32 i32 i32 i32 i32)))
 (import "host" "host_present" (func $shared/host/present (param i32)))
 (import "host" "host_log_i32" (func $shared/host/logI32 (param i32 i32)))
 (global $shared/host/DISPLAY_WIDTH i32 (i32.const 960))
 (global $shared/host/DISPLAY_HEIGHT i32 (i32.const 540))
 (global $shared/host/BLACK i32 (i32.const 0))
 (global $shared/host/WHITE i32 (i32.const 16777215))
 (global $shared/host/MID_GRAY i32 (i32.const 9211020))
 (global $shared/host/LIGHT_GRAY i32 (i32.const 14277081))
 (memory $0 0)
 (table $0 1 1 funcref)
 (elem $0 (i32.const 1))
 (export "run" (func $hello-frame/assembly/index/run))
 (export "memory" (memory $0))
 (func $hello-frame/assembly/index/run (result i32)
  global.get $shared/host/WHITE
  call $shared/host/screenClear
  i32.const 16
  i32.const 16
  global.get $shared/host/DISPLAY_WIDTH
  i32.const 32
  i32.sub
  global.get $shared/host/DISPLAY_HEIGHT
  i32.const 32
  i32.sub
  global.get $shared/host/BLACK
  call $shared/host/drawRect
  i32.const 28
  i32.const 28
  global.get $shared/host/DISPLAY_WIDTH
  i32.const 56
  i32.sub
  global.get $shared/host/DISPLAY_HEIGHT
  i32.const 56
  i32.sub
  global.get $shared/host/MID_GRAY
  call $shared/host/drawRect
  i32.const 56
  i32.const 72
  i32.const 260
  i32.const 92
  global.get $shared/host/BLACK
  call $shared/host/fillRect
  i32.const 70
  i32.const 86
  i32.const 232
  i32.const 64
  global.get $shared/host/WHITE
  call $shared/host/fillRect
  global.get $shared/host/DISPLAY_WIDTH
  i32.const 320
  i32.sub
  global.get $shared/host/DISPLAY_HEIGHT
  i32.const 164
  i32.sub
  i32.const 248
  i32.const 76
  global.get $shared/host/MID_GRAY
  call $shared/host/fillRect
  global.get $shared/host/DISPLAY_WIDTH
  i32.const 320
  i32.sub
  global.get $shared/host/DISPLAY_HEIGHT
  i32.const 164
  i32.sub
  i32.const 248
  i32.const 76
  global.get $shared/host/BLACK
  call $shared/host/drawRect
  i32.const 1
  call $shared/host/present
  i32.const 1
  i32.const 79
  call $shared/host/logI32
  i32.const 0
  return
 )
)
