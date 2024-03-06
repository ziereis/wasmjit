(module
  (import "test" "test" (func $test (param i32)))

  (func $addconst (param $a i32) (param $b i32) (result i32)
    (local $c i32)
    i32.const 5
    local.set $c
    local.get $a
    local.get $b
    i32.add
    local.get $c
    i32.add)
  (func $main (result i32)
    i32.const 1
    i32.const 2
    call $addconst)
  (export "main" (func $main))
  (export "addconst" (func $addconst))
)
