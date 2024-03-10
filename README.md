# Building
```bash
git submodule update --init --recursive
mkdir build 
cd build
cmake .. -DASMJIT_STATIC=1
```