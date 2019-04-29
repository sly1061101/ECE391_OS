#ifndef _TERMINAL_H_
#define _TERMINAL_H_

#include "types.h"
#include "keyboard.h"

#define TERMINAL_BUFFER_CAPACITY (KEYBOARD_BUFFER_CAPACITY * 10)
#define TERMINAL_NUM 3

#define TERMINAL_INACTIVE 0
#define TERMINAL_ACTIVE 1

#ifndef ASM

extern uint8_t video_mem_backstore[TERMINAL_NUM][4 * 1024];
extern int screen_x_backstore[TERMINAL_NUM];
extern int screen_y_backstore[TERMINAL_NUM];

extern int get_display_terminal();

extern void terminal_init(); 
// Set terminal state, 0 inactive, 1 active.
extern void set_terminal_state(uint32_t terminal_id, uint32_t state);
// Get next terminal which still hasn't been used.
extern int32_t get_next_inactive_terminal();

extern void terminal_switch(uint32_t terminal_id);

extern int terminal_buffer_write(unsigned char *buf, int size);

/* Initialize terminal stuff(or nothing), return 0 */
extern int terminal_open();

/* Clear any terminal specific variables(or do nothing), return 0 */
extern int terminal_close(int32_t fd);

/* Read FROM the keyboard buffer into buf, return number of bytes read */
extern int terminal_read(int32_t fd, unsigned char* buf, int size);

/* Write TO the screen from buff, return number of bytes written or -1 */
extern int terminal_write(int32_t fd, unsigned char* buf, int size);

#endif // ASM

#endif /* _TERMINAL_H_ */
