TARGET := riscv32-unknown-linux-gnu-

CC := $(TARGET)cc
CFLAGS := -march=rv32im_zicsr_zihintpause -mabi=ilp32 -O2 -ffreestanding
LD := $(CC)
LDFLAGS := -nostdlib -static -Wl,-Ttext,0

OBJCOPY := $(TARGET)objcopy
OBJDUMP := $(TARGET)objdump

objects := start.o main.o

.PHONY : all clean dump run

all: a.bin

clean:
	$(RM) a.bin a.out $(objects)

run: a.bin
	cargo run --manifest-path ../emcpu/Cargo.toml --example execute-binary -- a.bin

dump: a.out
	$(OBJDUMP) -d $<

a.bin: a.out
	$(OBJCOPY) -O binary -j .text -j .rodata -j .data $< $@

a.out: $(objects)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)

%.o: %.s
	$(CC) -c $(CFLAGS) -o $@ $<

%.o: %.S
	$(CC) -c $(CFLAGS) -o $@ $<

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<
