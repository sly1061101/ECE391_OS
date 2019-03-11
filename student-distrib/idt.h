#ifndef IDT_H
#define IDT_H

#include "x86_desc.h"

#ifndef ASM

// Jump table for all interrupt hanlders.
extern void (*interrupt_handler[NUM_VEC])();

extern void idt_init();

#endif

#endif /* IDT_H */
