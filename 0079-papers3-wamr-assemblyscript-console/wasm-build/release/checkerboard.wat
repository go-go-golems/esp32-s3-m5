(module
 (type $0 (func (param i32)))
 (type $1 (func (param i32 i32 i32 i32 i32)))
 (type $2 (func (param i32 i32)))
 (type $3 (func (result i32)))
 (import "host" "host_screen_clear" (func $shared/host/screenClear (param i32)))
 (import "host" "host_fill_rect" (func $shared/host/fillRect (param i32 i32 i32 i32 i32)))
 (import "host" "host_draw_rect" (func $shared/host/drawRect (param i32 i32 i32 i32 i32)))
 (import "host" "host_present" (func $shared/host/present (param i32)))
 (import "host" "host_log_i32" (func $shared/host/logI32 (param i32 i32)))
 (memory $0 0)
 (export "run" (func $checkerboard/assembly/index/run))
 (export "memory" (memory $0))
 (func $checkerboard/assembly/index/run (result i32)
  (local $0 i32)
  (local $1 i32)
  (local $2 i32)
  (local $3 i32)
  i32.const 16777215
  call $shared/host/screenClear
  loop $for-loop|0
   local.get $1
   i32.const 5
   i32.lt_s
   if
    i32.const 0
    local.set $0
    loop $for-loop|1
     local.get $0
     i32.const 10
     i32.lt_s
     if
      local.get $0
      i32.const 72
      i32.mul
      i32.const 96
      i32.add
      local.tee $2
      local.get $1
      i32.const 72
      i32.mul
      i32.const 84
      i32.add
      local.tee $3
      i32.const 72
      i32.const 72
      i32.const 9211020
      i32.const 14277081
      local.get $0
      local.get $1
      i32.add
      i32.const 1
      i32.and
      select
      call $shared/host/fillRect
      local.get $2
      local.get $3
      i32.const 72
      i32.const 72
      i32.const 0
      call $shared/host/drawRect
      local.get $0
      i32.const 1
      i32.add
      local.set $0
      br $for-loop|1
     end
    end
    local.get $1
    i32.const 1
    i32.add
    local.set $1
    br $for-loop|0
   end
  end
  i32.const 1
  call $shared/host/present
  i32.const 4
  i32.const 72
  call $shared/host/logI32
  i32.const 0
 )
)
