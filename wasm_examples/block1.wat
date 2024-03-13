(module
  (func $testBlock (param $x i32) (result i32)
    (block $blk  (result i32)
    local.get $x
    local.get $x
    i32.eqz
    br_if 1
    i32.const 100
    i32.add
  )
    i32.const 1000
    i32.add
  )
  (func $_start  (result i32)
    i32.const 0
    call $testBlock
  )
  (export "_start" (func $_start))
  (export "testBlock" (func $testBlock))
)
