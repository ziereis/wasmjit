CC = wasi-sdk-17.0/bin/clang
CFLAGS = -O1

SRC_FILES := $(wildcard *.c)
WASM_FILES := $(SRC_FILES:.c=.wasm)


all: $(WASM_FILES) $(WASM_FILES:.wasm=.wat)

%.wasm: %.c
	$(CC) $(CFLAGS) -o $@ $<


%.wat: %.wasm
	wasm2wat $< > $@

clean:
	rm -f $(WASM_FILES) *.wat
