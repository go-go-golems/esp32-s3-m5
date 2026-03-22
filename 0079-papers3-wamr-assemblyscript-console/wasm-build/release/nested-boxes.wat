(module
 (type $0 (func (param i32)))
 (type $1 (func (param i32 i32 i32 i32 i32)))
 (type $2 (func (param i32 i32)))
 (type $3 (func (result i32)))
 (import "host" "host_screen_clear" (func $shared/host/screenClear (param i32)))
 (import "host" "host_draw_rect" (func $shared/host/drawRect (param i32 i32 i32 i32 i32)))
 (import "host" "host_present" (func $shared/host/present (param i32)))
 (import "host" "host_log_i32" (func $shared/host/logI32 (param i32 i32)))
 (memory $0 0)
 (export "run" (func $nested-boxes/assembly/index/run))
 (export "memory" (memory $0))
 (func $nested-boxes/assembly/index/run (result i32)
  (local $0 i32)
  (local $1 i32)
  (local $2 i32)
  i32.const 16777215
  call $shared/host/screenClear
  i32.const 24
  local.set $0
  loop $for-loop|0
   local.get $1
   i32.const 9
   i32.lt_s
   if
    local.get $0
    local.get $0
    i32.const 960
    local.get $0
    i32.const 1
    i32.shl
    local.tee $2
    i32.sub
    i32.const 540
    local.get $2
    i32.sub
    local.get $1
    i32.const 8
    i32.eq
    if (result i32)
     i32.const 0
    else
     local.get $1
     i32.const 18
     i32.mul
     i32.const 24
     i32.add
     i32.const 255
     i32.and
     local.tee $2
     local.get $2
     i32.const 16
     i32.shl
     local.get $2
     i32.const 8
     i32.shl
     i32.or
     i32.or
    end
    call $shared/host/drawRect
    local.get $0
    i32.const 24
    i32.add
    local.set $0
    local.get $1
    i32.const 1
    i32.add
    local.set $1
    br $for-loop|0
   end
  end
  i32.const 1
  call $shared/host/present
  i32.const 2
  local.get $0
  call $shared/host/logI32
  i32.const 0
 )
)
