(module
  (import "test" "test" (func $test (param i32)))
  (func $conditional (param $x i32) (result i32)
    local.get $x
    i32.const 10
    i32.gt_s
    if (result i32)
      local.get $x
      i32.const 10
      i32.add
    else
      local.get $x
      i32.const 5
      i32.add
    end
    i32.const 100
    i32.add
  )
  (func $main (result i32)
    i32.const 12
    call $conditional)
  (export "main" (func $main))
  (export "conditional" (func $conditional))
)
