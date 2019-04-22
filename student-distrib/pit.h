#ifndef _PIT_H
#define _PIT_H

#include "lib.h"

#ifndef ASM

/* Programmable Interval Timer Initialization */
extern void pit_init (int32_t freq);

/* Pit interrupt handler*/
extern void pit_handler(void);

#endif

#endif

