#ifndef _TERMINAL_H_
#define _TERMINAL_H_

#include "types.h"
#include "keyboard.h"

#define TERMINAL_BUFFER_CAPACITY (KEYBOARD_BUFFER_CAPACITY * 10)
#define TERMINAL_NUM 3

#define TERMINAL_INACTIVE 0
#define TERMINAL_ACTIVE 1

#ifndef ASM

#define _4KB 4 * 1024

#define VAL_184     184
#define VAL_512     512
#define VAL_4096    4096
#define VAL_1024    1024
#define VAL_4       4
#define VIDEO       0xB8000
// Video memory backstorage space for background terminals.
extern uint8_t video_mem_backstore[TERMINAL_NUM][_4KB];

// Screen position backstorage for background terminals.
extern int screen_x_backstore[TERMINAL_NUM];
extern int screen_y_backstore[TERMINAL_NUM];

// Return display terminal
extern int get_display_terminal();

// Initialize terminal
extern void terminal_init(); 

// Set terminal state, 0 inactive, 1 active.
extern void set_terminal_state(uint32_t terminal_id, uint32_t state);

// Get next terminal which still hasn't been used.
extern int32_t get_next_inactive_terminal();

// Major function that handle terminal switching
extern void switch_terminal(uint32_t terminal_id);

// Write to terminal buffer
extern int terminal_buffer_write(unsigned char *buf, int size);

/* Initialize terminal stuff(or nothing), return 0 */
extern int terminal_open();

/* Clear any terminal specific variables(or do nothing), return 0 */
extern int terminal_close(int32_t fd);

/* Read FROM the keyboard buffer into buf, return number of bytes read */
extern int terminal_read(int32_t fd, unsigned char* buf, int size);

/* Write TO the screen from buff, return number of bytes written or -1 */
extern int terminal_write(int32_t fd, unsigned char* buf, int size);

// Similar to terminal_write but make sure we are writing to currently displayed terminal.
extern int terminal_write_display_terminal(int32_t fd, unsigned char* buf, int size);

#endif // ASM

#endif /* _TERMINAL_H_ */
