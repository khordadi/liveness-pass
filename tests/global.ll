; RUN: opt -load-liveness-plugin %shlibdir/libLiveness%shlibext -passes=liveness %s  | FileCheck %s

; Verifies that the popcorn correctly captures global variables.

@var = global i32 123

define i32 @foo() {
  ret i32 1
}

; CHECK-LABEL: BB %0
; CHECK-NEXT:       i32 123
