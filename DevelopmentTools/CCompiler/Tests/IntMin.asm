; location of global variables

; program start section
  call __global_scope_initialization
  call __function_main
  halt

__global_scope_initialization:
  push BP
  mov BP, SP
  mov SP, BP
  pop BP
  ret

__function_main:
  push BP
  mov BP, SP
  isub SP, 3
  mov R0, 0x80000000
  mov [BP-1], R0
  mov R0, 0x80000000
  mov [BP-2], R0
  mov R0, 0x80000000
  mov [BP-3], R0
__function_main_return:
  mov SP, BP
  pop BP
  ret

