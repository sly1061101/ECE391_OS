#ifndef _KEYBOARD_H
#define _KEYBOARD_H

#include "i8259.h"
#include "tests.h"
#include "lib.h"

#define KEYBOARD_IR_VEC 0x21
#define KEYBOARD_IRQ 0x01
#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64

/* Initialize the keyboard */
extern void keyboard_init();

/* Keyboard interrupt handler */
extern void keyboard_handler();

#endif /* _KEYBOARD_H */
