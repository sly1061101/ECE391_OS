#ifndef _KEYBOARD_H
#define _KEYBOARD_H

#include "i8259.h"
#include "tests.h"
#include "lib.h"

#define KEYBOARD_IR_VEC 0x21
#define KEYBOARD_IRQ 0x01
#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64
#define LOW_BIT_OFFSER 0x01

#define MAP_SIZE 128
#define KEYBOARD_BUFFER_CAPACITY 128
#define TERMINAL_BUFFER_CAPACITY (KEYBOARD_BUFFER_CAPACITY * 10)

#define LEFT_SHIFT 0x2A
#define LEFT_SHIFT_R 0xAA
#define RIGHT_SHIFT 0x36
#define RIGHT_SHIFT_R 0xB6
#define TAB 0x0F
#define CAPS_LOCK 0x3A
#define BACKSPACE 0x0E
#define ENTER 0x1C
#define CTRL 0x1D
#define ALT 0x38
#define EMPTY 0x00
#define ESC 0x01

#define PRESSED 1
#define RELEASED 0

/* The scancode for key release (`break') is obtained from it by setting the high order bit */
#define HIGH_ORDER_BIT_MASK 0x80

/* US Keyboard Layout for general cases*/
extern unsigned char keyboard_map[MAP_SIZE];

/* US Keyboard Layout for CAPSLOCK cases*/
extern unsigned char keyboard_map_caps[MAP_SIZE];

/* US Keyboard Layout for SHIFT cases*/
extern unsigned char keyboard_map_shift[MAP_SIZE];

/* Initialize the keyboard */
extern void keyboard_init();

/* Keyboard interrupt handler */
extern void keyboard_handler();

/* Helper fcuntion to handle special input cases */
extern char char_converter(unsigned char input);

/* Initialize terminal stuff(or nothing), return 0 */
extern int terminal_open();

/* Clear any terminal specific variables(or do nothing), return 0 */
extern int terminal_close(int32_t fd);

/* Read FROM the keyboard buffer into buf, return number of bytes read */
extern int terminal_read(int32_t fd, unsigned char* buf, int size);

/* Write TO the screen from buff, return number of bytes written or -1 */
extern int terminal_write(int32_t fd, unsigned char* buf, int size);




#endif /* _KEYBOARD_H */
