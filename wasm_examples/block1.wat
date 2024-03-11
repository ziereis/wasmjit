(module
  (import "test" "test" (func $test (param i32)))
  (func $testBlock (param $x i32) (result i32)
   local.get $x
   i32.eqz
    (block $outer (param i32) (result i32)
      (block $inner (param i32)
       br_if $inner
       i32.const 1
       br $outer
      )
    i32.const 2
    )
  )
  (export "testBlock" (func $testBlock))
)
