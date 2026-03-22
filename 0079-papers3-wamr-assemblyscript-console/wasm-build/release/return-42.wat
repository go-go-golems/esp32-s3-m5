(module
 (type $0 (func (result i32)))
 (memory $0 0)
 (export "run" (func $return-42/assembly/index/run))
 (export "memory" (memory $0))
 (func $return-42/assembly/index/run (result i32)
  i32.const 42
 )
)
