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
 (export "run" (func $bars/assembly/index/run))
 (export "memory" (memory $0))
 (func $bars/assembly/index/run (result i32)
  (local $0 i32)
  (local $1 i32)
  (local $2 i32)
  (local $3 i32)
  i32.const 16777215
  call $shared/host/screenClear
  i32.const 40
  i32.const 40
  i32.const 880
  i32.const 460
  i32.const 0
  call $shared/host/drawRect
  loop $for-loop|0
   local.get $0
   i32.const 10
   i32.lt_s
   if
    local.get $0
    i32.const 80
    i32.mul
    i32.const 72
    i32.add
    local.tee $2
    i32.const 452
    local.get $0
    i32.const 28
    i32.mul
    i32.const 48
    i32.add
    local.tee $1
    i32.sub
    local.tee $3
    i32.const 48
    local.get $1
    i32.const 14277081
    call $shared/host/fillRect
    local.get $2
    local.get $3
    i32.const 48
    local.get $1
    i32.const 0
    call $shared/host/drawRect
    local.get $0
    i32.const 1
    i32.add
    local.set $0
    br $for-loop|0
   end
  end
  i32.const 1
  call $shared/host/present
  i32.const 3
  i32.const 452
  call $shared/host/logI32
  i32.const 0
 )
)
