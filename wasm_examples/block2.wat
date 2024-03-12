(module
  (func $testBlock (param $x i32) (result i32)
    (block $blk (result i32)
      local.get $x
    )
  )
  (export "testBlock" (func $testBlock))
)
