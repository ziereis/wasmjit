(module
  (func $testBlock (param $x i32) (result i32)
    (block $blk0 (result i32)
      (block $blk1 (result i32)
        i32.const 10
        local.get $x
        i32.const 10
        i32.gt_s
        br_if 0
      )
      local.get $x
      i32.const 20
      i32.gt_s
      br_if $blk0
    )
    i32.const 42
  )
  (func $_start  (result i32)
    i32.const 100
    call $testBlock
  )
  (export "_start" (func $_start))
  (export "testBlock" (func $testBlock))
)
