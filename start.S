    .global _start
_start:
    lw gp, global_pointer_hack

    li t0, 0x8000
    add sp, gp, t0
    j start

    .section ".rodata"
global_pointer_hack:
    .word __global_pointer$
