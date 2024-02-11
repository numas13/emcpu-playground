    .global _start
_start:
    ld gp, global_pointer_hack

    li t0, 0x8000
    add sp, gp, t0
    j start

    .section ".rodata"
global_pointer_hack:
    .dword __global_pointer$
