    .global _start
_start:
    lw gp, global_pointer_hack

    lui a0, %hi(isr_entry)
    addi a0, a0, %lo(isr_entry)
    csrw mtvec, a0

    li t0, 1
    lui t1, %hi(value)
    sw t0, %lo(value)(t1)

    li t0, 0x8000
    add sp, gp, t0
    j start

isr_entry:
    addi sp, sp, -64
    sw a0, 0(sp)
    sw a1, 4(sp)
    sw a2, 8(sp)
    sw a3, 12(sp)
    sw a4, 16(sp)
    sw a5, 20(sp)
    sw a6, 24(sp)
    sw a7, 28(sp)
    sw t0, 32(sp)
    sw t1, 36(sp)
    sw t2, 40(sp)
    sw t3, 44(sp)
    sw t4, 48(sp)
    sw t5, 52(sp)
    sw t6, 56(sp)
    sw ra, 60(sp)
    csrrs a0, mcause, x0
    csrrs a1, mtval, x0
    csrrs a2, mepc, x0
    call isr
    csrrw x0, mepc, a0
    lw a0, 0(sp)
    lw a1, 4(sp)
    lw a2, 8(sp)
    lw a3, 12(sp)
    lw a4, 16(sp)
    lw a5, 20(sp)
    lw a6, 24(sp)
    lw a7, 28(sp)
    lw t0, 32(sp)
    lw t1, 36(sp)
    lw t2, 40(sp)
    lw t3, 44(sp)
    lw t4, 48(sp)
    lw t5, 52(sp)
    lw t6, 56(sp)
    lw ra, 60(sp)
    addi sp, sp, 64
    mret

    .section ".rodata"
global_pointer_hack:
    .word __global_pointer$
