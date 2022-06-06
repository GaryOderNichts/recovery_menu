#include "lolserial.h"
#include "imports.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>

#define LT_REG_BASE                   (0x0d800000)
#define LT_TIMER                      (LT_REG_BASE + 0x010)
#define LT_GPIO_ENABLE                (LT_REG_BASE + 0x0dc)
#define LT_GPIO_OUT                   (LT_REG_BASE + 0x0e0)
#define LT_GPIO_DIR                   (LT_REG_BASE + 0x0e4)
#define LT_GPIO_OWNER                 (LT_REG_BASE + 0x0fc)

#define LOLSERIAL_WAIT_TICKS 200

#if 0
#define LOLSERIAL_PIN 0x00000100 //GP_SENSORBAR
#else
#define LOLSERIAL_PIN 0x00010000 // GP_DEBUG0
#endif

void lolserial_lprint(const char *str, int len)
{
    /* setup output pin */
    *(volatile uint32_t*) LT_GPIO_OWNER &= ~LOLSERIAL_PIN;
    *(volatile uint32_t*) LT_GPIO_ENABLE |= LOLSERIAL_PIN;
    *(volatile uint32_t*) LT_GPIO_DIR |= LOLSERIAL_PIN;
    *(volatile uint32_t*) LT_GPIO_OUT |= LOLSERIAL_PIN;

    /* loop until null terminator or string end */
    for (const char *end = str + len; *str && (str != end); str++) {
        for (uint32_t bits = 0x200 | (*str << 1); bits; bits >>= 1) {
            /* set bit value */
            *(volatile uint32_t*) LT_GPIO_OUT = ((*(volatile uint32_t*) LT_GPIO_OUT) & ~LOLSERIAL_PIN) | ((bits & 1) ? LOLSERIAL_PIN : 0);

            /* wait ticks for bit */
            uint32_t now = *(volatile uint32_t*) LT_TIMER, then = now + LOLSERIAL_WAIT_TICKS;
            for (; then < now; now = *(volatile uint32_t*) LT_TIMER); /* wait overflow */
            for (; now < then; now = *(volatile uint32_t*) LT_TIMER); /* wait */
        }
    }
}

void lolserial_print(const char *str)
{
    lolserial_lprint(str, -1);
}

void lolserial_printf(const char* format, ...)
{
    int level = disable_interrupts();

    va_list args;
    va_start(args, format);

    char buffer[0x100];

    int len = vsnprintf(buffer, sizeof(buffer), format, args);
    lolserial_lprint(buffer, len);

    va_end(args);

    enable_interrupts(level);
}
