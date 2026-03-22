(module
 (type $0 (func (param i32)))
 (type $1 (func (param i32 i32 i32) (result i32)))
 (type $2 (func (param i32 i32 i32 i32 i32)))
 (type $3 (func (param i32 i32)))
 (type $4 (func (result i32)))
 (import "host" "host_screen_clear" (func $shared/host/screenClear (param i32)))
 (import "host" "host_draw_rect" (func $shared/host/drawRect (param i32 i32 i32 i32 i32)))
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
 (export "run" (func $nested-boxes/assembly/index/run))
 (export "memory" (memory $0))
 (func $shared/host/rgb (param $r i32) (param $g i32) (param $b i32) (result i32)
  local.get $r
  i32.const 255
  i32.and
  i32.const 16
  i32.shl
  local.get $g
  i32.const 255
  i32.and
  i32.const 8
  i32.shl
  i32.or
  local.get $b
  i32.const 255
  i32.and
  i32.or
  return
 )
 (func $nested-boxes/assembly/index/run (result i32)
  (local $inset i32)
  (local $i i32)
  global.get $shared/host/WHITE
  call $shared/host/screenClear
  i32.const 24
  local.set $inset
  i32.const 0
  local.set $i
  loop $for-loop|0
   local.get $i
   i32.const 9
   i32.lt_s
   if
    local.get $inset
    local.get $inset
    global.get $shared/host/DISPLAY_WIDTH
    local.get $inset
    i32.const 2
    i32.mul
    i32.sub
    global.get $shared/host/DISPLAY_HEIGHT
    local.get $inset
    i32.const 2
    i32.mul
    i32.sub
    local.get $i
    i32.const 8
    i32.eq
    if (result i32)
     global.get $shared/host/BLACK
    else
     i32.const 24
     local.get $i
     i32.const 18
     i32.mul
     i32.add
     i32.const 24
     local.get $i
     i32.const 18
     i32.mul
     i32.add
     i32.const 24
     local.get $i
     i32.const 18
     i32.mul
     i32.add
     call $shared/host/rgb
    end
    call $shared/host/drawRect
    local.get $inset
    i32.const 24
    i32.add
    local.set $inset
    local.get $i
    i32.const 1
    i32.add
    local.set $i
    br $for-loop|0
   end
  end
  i32.const 1
  call $shared/host/present
  i32.const 2
  local.get $inset
  call $shared/host/logI32
  i32.const 0
  return
 )
)
