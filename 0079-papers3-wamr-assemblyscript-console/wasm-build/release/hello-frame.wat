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
 (memory $0 0)
 (export "run" (func $hello-frame/assembly/index/run))
 (export "memory" (memory $0))
 (func $hello-frame/assembly/index/run (result i32)
  i32.const 16777215
  call $shared/host/screenClear
  i32.const 16
  i32.const 16
  i32.const 928
  i32.const 508
  i32.const 0
  call $shared/host/drawRect
  i32.const 28
  i32.const 28
  i32.const 904
  i32.const 484
  i32.const 9211020
  call $shared/host/drawRect
  i32.const 56
  i32.const 72
  i32.const 260
  i32.const 92
  i32.const 0
  call $shared/host/fillRect
  i32.const 70
  i32.const 86
  i32.const 232
  i32.const 64
  i32.const 16777215
  call $shared/host/fillRect
  i32.const 640
  i32.const 376
  i32.const 248
  i32.const 76
  i32.const 9211020
  call $shared/host/fillRect
  i32.const 640
  i32.const 376
  i32.const 248
  i32.const 76
  i32.const 0
  call $shared/host/drawRect
  i32.const 1
  call $shared/host/present
  i32.const 1
  i32.const 79
  call $shared/host/logI32
  i32.const 0
 )
)
