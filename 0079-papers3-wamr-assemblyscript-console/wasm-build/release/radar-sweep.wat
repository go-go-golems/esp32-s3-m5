(module
 (type $0 (func (param i32)))
 (type $1 (func (param i32 i32 i32 i32 i32)))
 (type $2 (func (param i32 i32 i32) (result i32)))
 (type $3 (func (param i32 i32)))
 (type $4 (func (result i32)))
 (import "host" "host_screen_clear" (func $shared/host/screenClear (param i32)))
 (import "host" "host_draw_rect" (func $shared/host/drawRect (param i32 i32 i32 i32 i32)))
 (import "host" "host_fill_rect" (func $shared/host/fillRect (param i32 i32 i32 i32 i32)))
 (import "host" "host_present" (func $shared/host/present (param i32)))
 (import "host" "host_log_i32" (func $shared/host/logI32 (param i32 i32)))
 (memory $0 0)
 (export "run" (func $radar-sweep/assembly/index/run))
 (export "memory" (memory $0))
 (func $shared/host/rgb (param $0 i32) (param $1 i32) (param $2 i32) (result i32)
  local.get $2
  i32.const 255
  i32.and
  local.get $0
  i32.const 255
  i32.and
  i32.const 16
  i32.shl
  local.get $1
  i32.const 255
  i32.and
  i32.const 8
  i32.shl
  i32.or
  i32.or
 )
 (func $radar-sweep/assembly/index/run (result i32)
  (local $0 i32)
  (local $1 i32)
  i32.const 16777215
  call $shared/host/screenClear
  i32.const 300
  i32.const 90
  i32.const 360
  i32.const 360
  i32.const 0
  call $shared/host/drawRect
  i32.const 324
  i32.const 114
  i32.const 312
  i32.const 312
  i32.const 0
  call $shared/host/drawRect
  loop $for-loop|0
   local.get $0
   i32.const 8
   i32.lt_s
   if
    local.get $0
    i32.const 42
    i32.mul
    i32.const 316
    i32.add
    i32.const 270
    block $__inlined_func$shared/host/clamp (result i32)
     i32.const 40
     local.get $0
     i32.const 18
     i32.mul
     i32.const 40
     i32.add
     local.tee $1
     i32.const 40
     i32.lt_s
     br_if $__inlined_func$shared/host/clamp
     drop
     i32.const 328
     local.get $1
     i32.const 328
     i32.gt_s
     br_if $__inlined_func$shared/host/clamp
     drop
     local.get $1
    end
    local.tee $1
    i32.const 2
    i32.div_s
    i32.sub
    i32.const 18
    local.get $1
    local.get $0
    i32.const 20
    i32.mul
    i32.const 30
    i32.add
    local.tee $1
    local.get $1
    local.get $1
    call $shared/host/rgb
    call $shared/host/fillRect
    local.get $0
    i32.const 1
    i32.add
    local.set $0
    br $for-loop|0
   end
  end
  i32.const 476
  i32.const 98
  i32.const 8
  i32.const 344
  i32.const 0
  call $shared/host/fillRect
  i32.const 308
  i32.const 266
  i32.const 344
  i32.const 8
  i32.const 0
  call $shared/host/fillRect
  i32.const 0
  local.set $0
  loop $for-loop|1
   local.get $0
   i32.const 6
   i32.lt_s
   if
    local.get $0
    i32.const 26
    i32.mul
    i32.const 24
    i32.add
    local.tee $1
    i32.const 480
    i32.add
    i32.const 270
    local.get $1
    i32.sub
    i32.const 12
    i32.const 12
    local.get $0
    i32.const 25
    i32.mul
    i32.const 40
    i32.add
    local.tee $1
    local.get $1
    local.get $1
    call $shared/host/rgb
    call $shared/host/fillRect
    local.get $0
    i32.const 1
    i32.add
    local.set $0
    br $for-loop|1
   end
  end
  i32.const 1
  call $shared/host/present
  i32.const 5
  i32.const 180
  call $shared/host/logI32
  i32.const 0
 )
)
