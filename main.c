#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

typedef unsigned long size_t;
typedef signed long ssize_t;

#define EXCP_INSN_ADDR_MISALIGNED 0
#define EXCP_ILLEGAL_INSN 2
#define EXCP_BREAKPOINT 3
#define EXCP_LOAD_ACCESS_FAULT 5
#define EXCP_STORE_AMO_ACCESS_FAULT 7

#define INTERRUPT(i) ((1 << 31) | (i))
#define INTERRUPT_MACHINE_EXTERNAL  INTERRUPT(11)
#define INTERRUPT_DEV(i)            INTERRUPT(16 + (i))
#define INTERRUPT_STDIN INTERRUPT_DEV(0)

#define MSTATUS_MIE (1 << 3)

#define MIE_SS (1 << 1)
#define MIE_MS (1 << 3)
#define MIE_ST (1 << 5)
#define MIE_MT (1 << 7)
#define MIE_SE (1 << 9)
#define MIE_ME (1 << 11)

#define MEI_EXTERNAL_INT(i) (1 << (16 + (i)))
#define MEI_DEVICE_STDIN MEI_EXTERNAL_INT(0)

#define RETURN_NEVER    __attribute__((noreturn))
#define INLINE_ALWAYS   __attribute__((always_inline))
#define INLINE_NEVER    __attribute__((noinline))
#define INTERRUPT_HANDLER(mode) __attribute__((interrupt(mode)))

static volatile int pos = 0;
static char buf[128];
static int test = 0;

extern char *__bss_start;
extern char *_edata;
extern char *_end;

#define csr(reg) \
    ({ \
        uint32_t ret; \
        asm volatile("csrrs %0, " #reg ", x0" : "=r"(ret) ); \
        ret; \
    })

#define csrw(reg, v) asm volatile("csrrw x0, " #reg ", %0" : : "r"(v) )
#define csrs(reg, v) asm volatile("csrrs x0, " #reg ", %0" : : "r"(v) )
#define csrc(reg, v) asm volatile("csrrc x0, " #reg ", %0" : : "r"(v) )

INLINE_ALWAYS static inline void pause() { asm volatile("pause"); }
INLINE_ALWAYS static inline void ebreak() { asm volatile("ebreak"); }
INLINE_ALWAYS static inline void wfi() { asm volatile("wfi"); }
INLINE_ALWAYS static inline void set_mie(int mask) { csrw(mie, mask); }
INLINE_ALWAYS static inline void sti() { csrs(mstatus, MSTATUS_MIE); }
INLINE_ALWAYS static inline void cli() { csrc(mstatus, MSTATUS_MIE); }

RETURN_NEVER
static void halt() {
    for (;;) {
        wfi();
    }
}

#define TERMINAL_DEVICE 0

INLINE_ALWAYS
static inline volatile uint32_t * dev_mem(int id) {
    return (uint32_t *) (0x80000000UL + id * 0x1000);
}

INLINE_ALWAYS
static inline void stdout_putc(char c) {
    dev_mem(TERMINAL_DEVICE)[0] = c;
}

INLINE_ALWAYS
static inline char stdin_getc() {
    return dev_mem(TERMINAL_DEVICE)[0];
}

static int stdout_puts(const char *s) {
    int n;
    for (n = 0; s[n]; ++n) {
        stdout_putc(s[n]);
    }
    return n;
}

size_t strlen(const char *s) {
    int i = 0;
    for (; s[i]; ++i)
        ;
    return i;
}

int isdigit(int c) {
    return c >= '0' && c <= '9';
}

static void reverse(char *s, char *e) {
    for (; s < e; ++s, --e) {
        int t = *s;
        *s = *e;
        *e = t;
    }
}

int itoa(int n, char *nptr) {
    int i = 0, len = 0;

    if (n == 0) {
        nptr[0] = '0';
        nptr[1] = 0;
        return 1;
    }

    if (n < 0) {
        nptr[0] = '-';
        i = len = 1;
        n = -n;
    }

    for (; n; ++len, n /= 10) {
        int r = n % 10;
        nptr[len] = '0' + r;
    }

    nptr[len] = 0;

    reverse(&nptr[i], &nptr[len - 1]);

    return len;
}

int puts(const char *s) {
    int n = stdout_puts(s);
    stdout_putc('\n');
    return n + 1;
}

int printf(const char *fmt, ...) {
    int n = 0;
    bool alt = false, zero = false;
    va_list ap;
    unsigned int x;
    int i, width;
    char buf[64];
    const char *s;

    va_start(ap, fmt);
    while (*fmt) {
        if (*fmt != '%') {
            stdout_putc(*fmt);
            n += 1;
            fmt += 1;
            continue;
        }

        fmt += 1;

        for (bool stop = false; !stop;) {
            switch (*fmt) {
            case '#':
                alt = true;
                fmt += 1;
                break;
            case '0':
                zero = true;
                fmt += 1;
                break;
            default:
                stop = true;
                break;
            }
        }

        width = 0;
        for (; isdigit(*fmt); ++fmt) {
            width *= 10;
            width += *fmt - '0';
        }

        switch (*fmt) {
        case 'c':
            stdout_putc(va_arg(ap, int));
            n += 1;
            break;
        case 's':
            s = va_arg(ap, const char *);
            if (width > 0) {
                i = strlen(s);
                for (; i < width; ++i) {
                    stdout_putc(' ');
                    n += 1;
                }
            }
            n += stdout_puts(s);
            break;
        case 'u': // TODO: printf %u
        case 'd':
            i = itoa(va_arg(ap, int), buf);
            for (; i < width; ++i) {
                stdout_putc(' ');
                n += 1;
            }
            n += stdout_puts(buf);
            break;
        case 'x':
            x = va_arg(ap, unsigned int);
            i = 8;

            if (alt) {
                width -= 2;
            }

            for (; i > 0; --i) {
                if (((x >> ((i - 1) * 4)) & 0xf) != 0) {
                    break;
                }
            }

            if (zero) {
                if (alt) {
                    stdout_puts("0x");
                    n += 2;
                }
                for (int j = 0; j < (width - i); ++j) {
                    stdout_putc('0');
                    n += 1;
                }
            } else {
                for (int j = 0; j < (width - i); ++j) {
                    stdout_putc(' ');
                    n += 1;
                }
                if (alt) {
                    stdout_puts("0x");
                    n += 2;
                }
            } 

            for (int j = i; j > 0; --j) {
                int c = (x >> ((j - 1) * 4)) & 0xf; 
                stdout_putc(c + (c < 10 ? '0' : ('a' - 10)));
                n += 1;
            }
            break;
        case '\0':
            break;
        case '%':
            n += 1;
            stdout_putc('%');
            break;
        default:
            n += 2;
            stdout_putc('%');
            stdout_putc(*fmt);
            break;
        }
        fmt += 1;
    }
    va_end(ap);

    return n;
}

static int strcmp(const char *s1, const char *s2) {
    for (; *s1 && *s2 && *s1 == *s2; s1++, s2++)
        ;
    return *s1 - *s2;
}

void *memset(void *s, int c, size_t n) {
    char *p = s;
    for (size_t i = 0; i < n; ++i) {
        p[i] = c;
    }
    return s;
}

static void handle_command(char *cmd) {
    if (strcmp(cmd, "test") == 0) {
        test = 1;
    } else if (strcmp(cmd, "hello") == 0) {
        puts("Hello, user!");
    } else if (strcmp(cmd, "illop") == 0) {
        asm(".word 0");
    } else if (strcmp(cmd, "opene2k") == 0) {
        puts("Какое нафиг OpenE2K!? Это RISC-V!!!");
    } else if (strcmp(cmd, "halt") == 0) {
        puts("halt...");
        halt();
    } else {
        printf("command not found: `%s`\n", cmd);
    }
}

static long handle_excp(long cause, long val, long npc) {
    switch (cause) {
    case EXCP_INSN_ADDR_MISALIGNED:
        puts("misaligned instruction address");
        break;
    case EXCP_ILLEGAL_INSN:
        puts("illegal instruction");
        break;
    case EXCP_LOAD_ACCESS_FAULT:
        puts("load access fault");
        break;
    case EXCP_STORE_AMO_ACCESS_FAULT:
        puts("store/amo access fault");
        break;
    case EXCP_BREAKPOINT:
        puts("breakpoint");
        return npc + 4;
    default:
        puts("unknown exception");
        break;
    }
    halt();
}

static void handle_device_stdin() {
    char c;
    do {
        c = stdin_getc();
        buf[pos++] = c;
    } while(c);
}

static long handle_int(long cause, long npc) {
    switch (cause) {
    case INTERRUPT_STDIN:
        handle_device_stdin();
        csrc(mip, 1 << (INTERRUPT_STDIN & 63));
        break;
    default:
        printf("unimplemented interrupt %d\n", (int) cause & 0x7fffffff);
        halt();
    }
    return npc;
}

INTERRUPT_HANDLER("machine")
void isr_entry(void) {
    long cause = csr(mcause);
    long val = csr(mtval);
    long epc = csr(mepc);

    if (cause >= 0) {
        epc = handle_excp(cause, val, epc);
    } else {
        epc = handle_int(cause, epc);
    }

    csrw(mepc, epc);
}

void start() {
    memset(__bss_start, 0, _end - __bss_start);

    csrw(mtvec, isr_entry);
    set_mie(~0);
    sti();

    // printf("%%s     = \"%s\"\n", "hello");
    // printf("%%10s   = \"%10s\"\n", "hello");
    // printf("%%d     = \"%d\"\n", -1234);
    // printf("%%10u   = \"%10u\"\n", -1234);
    // printf("%%x     = \"%x\"\n", 0xbeef);
    // printf("%%016x  = \"%016x\"\n", 0xbeef);
    // printf("%%#16x  = \"%#16x\"\n", 0xbeef);
    // printf("%%#016x = \"%#016x\"\n", 0xbeef);

    for (;;) {
        printf("[%d] emcpu $ ", csr(minstret));
        wfi();

        cli();
        if (pos > 1 && buf[pos - 2] == '\n') {
            buf[pos - 2] = 0;
            handle_command(buf);
            pos = 0;
        }
        sti();

        if (test) {
            test = 0;
            volatile int *p = (int *) 0x1000000;
            *p += 1;
        }
    }
}
