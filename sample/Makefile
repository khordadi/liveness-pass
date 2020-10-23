LLVM_PATH := ${LLVM_PATH}

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
	LIB_EXT = so
endif
ifeq ($(UNAME_S),Darwin)
	LIB_EXT = dylib
endif

all: pass main
	$(LLVM_PATH)/bin/opt -load-pass-plugin ../build/libLiveness.$(LIB_EXT) -passes=liveness -disable-output main.ll

pass: ../Liveness.cpp ../CMakeLists.txt
	cmake --build ../build --target Popcorn -- -j 4

main: main.c
	clang -emit-llvm -S -o main.ll main.c

clean:
	rm -rf main.ll main.s main
