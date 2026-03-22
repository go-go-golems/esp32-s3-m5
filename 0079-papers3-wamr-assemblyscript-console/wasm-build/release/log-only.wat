(module
 (type $0 (func (param i32 i32)))
 (type $1 (func (result i32)))
 (import "host" "host_log_i32" (func $shared/host/logI32 (param i32 i32)))
 (memory $0 0)
 (export "run" (func $log-only/assembly/index/run))
 (export "memory" (memory $0))
 (func $log-only/assembly/index/run (result i32)
  i32.const 9
  i32.const 42
  call $shared/host/logI32
  i32.const 42
 )
)
