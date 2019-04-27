#ifndef IDT_H
#define IDT_H

#include "x86_desc.h"

#define PIT_VEC_NUM 0x20
#define KEYBOARD_VEC_NUM 0x21
#define RTC_VEC_NUM 0x28

#ifndef ASM

// Jump table for all interrupt hanlders. Last one is default handler.
extern void (*interrupt_handler[NUM_VEC + 1])();

extern void idt_init();

#endif

#endif /* IDT_H */
