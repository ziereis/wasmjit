(module
  (import "test" "test" (func $test (param i32)))
  (func $add (param $a i32) (param $b i32) (result i32)
    local.get $a
    local.get $b
    i32.add)

  (func $addconst (param $a i32) (param $b i32) (result i32)
    (local $c i32)
    i32.const 5
    local.set $c
    local.get $a
    local.get $b
    i32.add
    local.get $c
    i32.add)
  (export "addconst" (func $addconst))
  (export "add" (func $add))
)
