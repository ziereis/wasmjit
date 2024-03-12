(module
  (func $testBlock (param $x i32) (result i32)
    (block $blk  (result i32)
    local.get $x
    local.get $x
    i32.eqz
    br_if 0
    i32.const 100
    i32.add
  )
  )
  (export "testBlock" (func $testBlock))
)
