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
 (global $shared/host/DISPLAY_WIDTH i32 (i32.const 960))
 (global $shared/host/DISPLAY_HEIGHT i32 (i32.const 540))
 (global $shared/host/BLACK i32 (i32.const 0))
 (global $shared/host/WHITE i32 (i32.const 16777215))
 (global $shared/host/MID_GRAY i32 (i32.const 9211020))
 (global $shared/host/LIGHT_GRAY i32 (i32.const 14277081))
 (memory $0 0)
 (table $0 1 1 funcref)
 (elem $0 (i32.const 1))
 (export "run" (func $checkerboard/assembly/index/run))
 (export "memory" (memory $0))
 (func $checkerboard/assembly/index/run (result i32)
  (local $row i32)
  (local $col i32)
  (local $x i32)
  (local $y i32)
  (local $color i32)
  global.get $shared/host/WHITE
  call $shared/host/screenClear
  i32.const 0
  local.set $row
  loop $for-loop|0
   local.get $row
   i32.const 5
   i32.lt_s
   if
    i32.const 0
    local.set $col
    loop $for-loop|1
     local.get $col
     i32.const 10
     i32.lt_s
     if
      i32.const 96
      local.get $col
      i32.const 72
      i32.mul
      i32.add
      local.set $x
      i32.const 84
      local.get $row
      i32.const 72
      i32.mul
      i32.add
      local.set $y
      local.get $row
      local.get $col
      i32.add
      i32.const 1
      i32.and
      i32.const 0
      i32.eq
      if (result i32)
       global.get $shared/host/LIGHT_GRAY
      else
       global.get $shared/host/MID_GRAY
      end
      local.set $color
      local.get $x
      local.get $y
      i32.const 72
      i32.const 72
      local.get $color
      call $shared/host/fillRect
      local.get $x
      local.get $y
      i32.const 72
      i32.const 72
      global.get $shared/host/BLACK
      call $shared/host/drawRect
      local.get $col
      i32.const 1
      i32.add
      local.set $col
      br $for-loop|1
     end
    end
    local.get $row
    i32.const 1
    i32.add
    local.set $row
    br $for-loop|0
   end
  end
  i32.const 1
  call $shared/host/present
  i32.const 4
  i32.const 72
  call $shared/host/logI32
  i32.const 0
  return
 )
)
