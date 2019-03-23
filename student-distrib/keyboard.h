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
#define BUFFER_SIZE 128

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

#define PRESSED 1
#define RELEASED 0
#define READY 1
#define NOT_READY 0
/* The scancode for key release (`break') is obtained from it by setting the high order bit */
#define HIGH_ORDER_BIT_MASK 0x80

// Scancode table used to layout a standard US keyboard.
// copied from http://www.osdever.net/bkerndev/Docs/keyboard.htm
static unsigned char keyboard_map[MAP_SIZE] =
    {
        0, 27, '1', '2', '3', '4', '5', '6', '7', '8',    /* 9 */
        '9', '0', '-', '=', '\b',                         /* Backspace */
        '\t',                                             /* Tab */
        'q', 'w', 'e', 'r',                               /* 19 */
        't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',     /* Enter key */
        0,                                                /* 29   - Control */
        'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', /* 39 */
        '\'', '`', 0,                                     /* Left shift */
        '\\', 'z', 'x', 'c', 'v', 'b', 'n',               /* 49 */
        'm', ',', '.', '/', 0,                            /* Right shift */
        '*',
        0,   /* Alt */
        ' ', /* Space bar */
        0,   /* Caps lock */
        0,   /* 59 - F1 key ... > */
        0, 0, 0, 0, 0, 0, 0, 0,
        0, /* < ... F10 */
        0, /* 69 - Num lock*/
        0, /* Scroll Lock */
        0, /* Home key */
        0, /* Up Arrow */
        0, /* Page Up */
        '-',
        0, /* Left Arrow */
        0,
        0, /* Right Arrow */
        '+',
        0, /* 79 - End key*/
        0, /* Down Arrow */
        0, /* Page Down */
        0, /* Insert Key */
        0, /* Delete Key */
        0, 0, 0,
        0, /* F11 Key */
        0, /* F12 Key */
        0, /* All other keys are undefined */
};

// Scancode for keyboard when TAB is pressed
static unsigned char keyboard_map_caps[MAP_SIZE] =
    {
        0, 27, '1', '2', '3', '4', '5', '6', '7', '8',    /* 9 */
        '9', '0', '-', '=', '\b',                         /* Backspace */
        '\t',                                             /* Tab */
        'Q', 'W', 'E', 'R',                               /* 19 */
        'T', 'Y', 'U', 'I', 'O', 'P', '[', ']', '\n',     /* Enter key */
        0,                                                /* 29   - Control */
        'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', /* 39 */
        '\'', '`', 0,                                     /* Left shift */
        '\\', 'Z', 'X', 'C', 'V', 'B', 'N',               /* 49 */
        'M', ',', '.', '/', 0,                            /* Right shift */
        '*',
        0,   /* Alt */
        ' ', /* Space bar */
        0,   /* Caps lock */
        0,   /* 59 - F1 key ... > */
        0, 0, 0, 0, 0, 0, 0, 0,
        0, /* < ... F10 */
        0, /* 69 - Num lock*/
        0, /* Scroll Lock */
        0, /* Home key */
        0, /* Up Arrow */
        0, /* Page Up */
        '-',
        0, /* Left Arrow */
        0,
        0, /* Right Arrow */
        '+',
        0, /* 79 - End key*/
        0, /* Down Arrow */
        0, /* Page Down */
        0, /* Insert Key */
        0, /* Delete Key */
        0, 0, 0,
        0, /* F11 Key */
        0, /* F12 Key */
        0, /* All other keys are undefined */
};

// Scancode for keyboard when SHIFT is pressed
static unsigned char keyboard_map_shift[MAP_SIZE] =
    {
        0, 27, '!', '@', '#', '$', '%', '^', '&', '*',    /* 9 */
        '(', ')', '_', '+', '\b',                         /* Backspace */
        '\t',                                             /* Tab */
        'Q', 'W', 'E', 'R',                               /* 19 */
        'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',     /* Enter key */
        0,                                                /* 29   - Control */
        'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', /* 39 */
        '\'', '~', 0,                                     /* Left shift */
        '|', 'Z', 'X', 'C', 'V', 'B', 'N',                /* 49 */
        'M', '<', '>', '?', 0,                            /* Right shift */
        '*',
        0,   /* Alt */
        ' ', /* Space bar */
        0,   /* Caps lock */
        0,   /* 59 - F1 key ... > */
        0, 0, 0, 0, 0, 0, 0, 0,
        0, /* < ... F10 */
        0, /* 69 - Num lock*/
        0, /* Scroll Lock */
        0, /* Home key */
        0, /* Up Arrow */
        0, /* Page Up */
        '-',
        0, /* Left Arrow */
        0,
        0, /* Right Arrow */
        '+',
        0, /* 79 - End key*/
        0, /* Down Arrow */
        0, /* Page Down */
        0, /* Insert Key */
        0, /* Delete Key */
        0, 0, 0,
        0, /* F11 Key */
        0, /* F12 Key */
        0, /* All other keys are undefined */
};

/* Initialize the keyboard */
extern void keyboard_init();

/* Keyboard interrupt handler */
extern void keyboard_handler();

/* Helper fcuntion to handle special input cases */
extern char char_converter(unsigned char input);

/* Initialize terminal stuff(or nothing), return 0 */
extern int terminal_open();

/* Clear any terminal specific variables(or do nothing), return 0 */
extern int terminal_close();

/* Read FROM the keyboard buffer into buf, return number of bytes read */
extern int terminal_read(char* buf, int size);

/* Write TO the screen from buff, return number of bytes written or -1 */
extern int terminal_write(char* buf, int size);




#endif /* _KEYBOARD_H */
