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
 (global $shared/host/DISPLAY_WIDTH i32 (i32.const 960))
 (global $shared/host/DISPLAY_HEIGHT i32 (i32.const 540))
 (global $shared/host/BLACK i32 (i32.const 0))
 (global $shared/host/WHITE i32 (i32.const 16777215))
 (global $shared/host/MID_GRAY i32 (i32.const 9211020))
 (global $shared/host/LIGHT_GRAY i32 (i32.const 14277081))
 (memory $0 0)
 (table $0 1 1 funcref)
 (elem $0 (i32.const 1))
 (export "run" (func $radar-sweep/assembly/index/run))
 (export "memory" (memory $0))
 (func $shared/host/clamp (param $value i32) (param $minValue i32) (param $maxValue i32) (result i32)
  local.get $value
  local.get $minValue
  i32.lt_s
  if
   local.get $minValue
   return
  end
  local.get $value
  local.get $maxValue
  i32.gt_s
  if
   local.get $maxValue
   return
  end
  local.get $value
  return
 )
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
 (func $radar-sweep/assembly/index/run (result i32)
  (local $band i32)
  (local $bandX i32)
  (local $bandHeight i32)
  (local $i i32)
  (local $step i32)
  global.get $shared/host/WHITE
  call $shared/host/screenClear
  i32.const 480
  i32.const 180
  i32.sub
  i32.const 270
  i32.const 180
  i32.sub
  i32.const 180
  i32.const 2
  i32.mul
  i32.const 180
  i32.const 2
  i32.mul
  global.get $shared/host/BLACK
  call $shared/host/drawRect
  i32.const 480
  i32.const 180
  i32.sub
  i32.const 24
  i32.add
  i32.const 270
  i32.const 180
  i32.sub
  i32.const 24
  i32.add
  i32.const 180
  i32.const 24
  i32.sub
  i32.const 2
  i32.mul
  i32.const 180
  i32.const 24
  i32.sub
  i32.const 2
  i32.mul
  global.get $shared/host/BLACK
  call $shared/host/drawRect
  i32.const 0
  local.set $band
  loop $for-loop|0
   local.get $band
   i32.const 8
   i32.lt_s
   if
    i32.const 480
    i32.const 180
    i32.sub
    i32.const 16
    i32.add
    local.get $band
    i32.const 42
    i32.mul
    i32.add
    local.set $bandX
    i32.const 40
    local.get $band
    i32.const 18
    i32.mul
    i32.add
    i32.const 40
    i32.const 180
    i32.const 2
    i32.mul
    i32.const 32
    i32.sub
    call $shared/host/clamp
    local.set $bandHeight
    local.get $bandX
    i32.const 270
    local.get $bandHeight
    i32.const 2
    i32.div_s
    i32.sub
    i32.const 18
    local.get $bandHeight
    i32.const 30
    local.get $band
    i32.const 20
    i32.mul
    i32.add
    i32.const 30
    local.get $band
    i32.const 20
    i32.mul
    i32.add
    i32.const 30
    local.get $band
    i32.const 20
    i32.mul
    i32.add
    call $shared/host/rgb
    call $shared/host/fillRect
    local.get $band
    i32.const 1
    i32.add
    local.set $band
    br $for-loop|0
   end
  end
  i32.const 480
  i32.const 4
  i32.sub
  i32.const 270
  i32.const 180
  i32.sub
  i32.const 8
  i32.add
  i32.const 8
  i32.const 180
  i32.const 2
  i32.mul
  i32.const 16
  i32.sub
  global.get $shared/host/BLACK
  call $shared/host/fillRect
  i32.const 480
  i32.const 180
  i32.sub
  i32.const 8
  i32.add
  i32.const 270
  i32.const 4
  i32.sub
  i32.const 180
  i32.const 2
  i32.mul
  i32.const 16
  i32.sub
  i32.const 8
  global.get $shared/host/BLACK
  call $shared/host/fillRect
  i32.const 0
  local.set $i
  loop $for-loop|1
   local.get $i
   i32.const 6
   i32.lt_s
   if
    i32.const 24
    local.get $i
    i32.const 26
    i32.mul
    i32.add
    local.set $step
    i32.const 480
    local.get $step
    i32.add
    i32.const 270
    local.get $step
    i32.sub
    i32.const 12
    i32.const 12
    i32.const 40
    local.get $i
    i32.const 25
    i32.mul
    i32.add
    i32.const 40
    local.get $i
    i32.const 25
    i32.mul
    i32.add
    i32.const 40
    local.get $i
    i32.const 25
    i32.mul
    i32.add
    call $shared/host/rgb
    call $shared/host/fillRect
    local.get $i
    i32.const 1
    i32.add
    local.set $i
    br $for-loop|1
   end
  end
  i32.const 1
  call $shared/host/present
  i32.const 5
  i32.const 180
  call $shared/host/logI32
  i32.const 0
  return
 )
)
