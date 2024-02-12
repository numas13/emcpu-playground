    .global _start
_start:
    .option push
    .option norelax
0:
    auipc gp, %pcrel_hi(__global_pointer$)
    addi gp, gp, %pcrel_lo(0b)
    .option pop

    li t0, 0x8000
    add sp, gp, t0
    j start
