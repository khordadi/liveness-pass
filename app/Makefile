LLVM_PATH := ${LLVM_PATH}

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
	LIB_EXT = so
endif
ifeq ($(UNAME_S),Darwin)
	LIB_EXT = dylib
endif

all: pass main.ll
	$(LLVM_PATH)/bin/opt -load-pass-plugin ../build/libPopcorn.$(LIB_EXT) -passes=popcorn -disable-output main.ll

pass: ../Pass.cpp ../CMakeLists.txt
	cmake --build ../build --target Popcorn -- -j 4

main.ll: main.c
	clang -O0 -emit-llvm -S -o main.ll main.c

clean:
	rm -rf main.ll main.s main


#main: build/main.s
#	clang -o build/main build/main.s
#
#main.s: build/main.ll
#	llc -o build/main.s build/main.ll
