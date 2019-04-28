#include "terminal.h"
#include "paging.h"
#include "lib.h"

unsigned char terminal_buffer[TERMINAL_NUM][TERMINAL_BUFFER_CAPACITY];
int terminal_buffer_size[TERMINAL_NUM];

uint8_t video_mem_backstore[TERMINAL_NUM][4 * 1024] __attribute__((aligned(4096)));
int screen_x_backstore[TERMINAL_NUM];
int screen_y_backstore[TERMINAL_NUM];

int display_terminal;

int get_display_terminal() {
    return display_terminal;
}

// 0 inactive, 1 active.
int32_t terminal_state[TERMINAL_NUM];

void set_terminal_state(uint32_t terminal_id, uint32_t state) {
  terminal_state[terminal_id] = state;
}

extern int32_t get_next_inactive_terminal() {
  int32_t i;
  for(i = 0; i < TERMINAL_NUM; ++i) {
    if(terminal_state[i] == TERMINAL_INACTIVE)
      return i;
  }
  return -1;
}

void terminal_init() {
  int i;
  for (i = 0 ; i < TERMINAL_NUM; i++) {
    terminal_buffer_size[i] = 0;
    terminal_state[i] = TERMINAL_INACTIVE;
    screen_x_backstore[i] = 0;
    screen_y_backstore[i] = 0;
    memset(video_mem_backstore[i], 0, 4 * 1024);
  }

  display_terminal = 0;
}

/* terminal_buffer_write
* Write something into the terminal buffer.
* Input: size for buffer and buf pointer
* Output: number of the movement 
* Side Effects: none
*/

int terminal_buffer_write(unsigned char *buf, int size) {
  if(buf == NULL || size < 0)
    return -1;

  // If terminal buffer does not have enough space to hold the input.
  if(TERMINAL_BUFFER_CAPACITY - terminal_buffer_size[display_terminal] < size)
    return -1;

  int i = 0;
  while(i < size) {
    int temp = terminal_buffer_size[display_terminal];
    terminal_buffer[display_terminal][temp] = buf[i];
    terminal_buffer_size[display_terminal]++;
    i++;
  }

  return i;
}

/* terminal_buffer_move
* Move the terminal buffer to the left by size
* Input: size for move
* Output: number of the movement 
* Side Effects: none
*/
int terminal_buffer_move(int size)
{
  if(size > terminal_buffer_size[get_current_pcb()->terminal_id])
    return -1;

  int i;
  for(i = 0; i < terminal_buffer_size[get_current_pcb()->terminal_id] - size; i++)
  {
    terminal_buffer[get_current_pcb()->terminal_id][i] = terminal_buffer[get_current_pcb()->terminal_id][i + size];
  }
  terminal_buffer_size[get_current_pcb()->terminal_id] -= size; 
  return i;
}

int32_t terminal_switch(uint32_t terminal_id) {
  backup_video_memory(video_mem_backstore[display_terminal]);
  load_video_memory(video_mem_backstore[terminal_id]);

  page_table_terminal_video_memory[display_terminal][184].page_base_address = (uint32_t)video_mem_backstore[display_terminal] >> 12;
  page_table_terminal_video_memory[terminal_id][184].page_base_address = 0xB8000 >> 12;

  load_page_directory(page_directory_program[get_current_pcb()->pid]);

  display_terminal = terminal_id;
}

/* terminal_open
 * Initialize terminal stuff(or nothing)
 * Inputs: None
 * Outputs: Return 0
 * Side Effects: none
 */
int terminal_open()
{
  return 0;
}

/* terminal_close
 * Clear any terminal specific variables(or do nothing)
 * Inputs: None
 * Outputs: Return 0
 * Side Effects: none
 */
int terminal_close(int32_t fd)
{
  return 0;
}

/* terminal_read
 * Read FROM the keyboard buffer into buf
 * Inputs: keyboard buffer
 * Outputs: number of bytes read
 * Side Effects: none
 */
int terminal_read(int32_t fd, unsigned char* buf, int size)
{
  // Only Stdin can be read from.
  if(fd != 0)
    return -1;

  if(size < 0)
    return -1;

  // Wait until terminal buffer is not empty.
  while(terminal_buffer_size[get_current_pcb()->terminal_id] == 0);

  int i;
  for(i = 0; i < size && i < terminal_buffer_size[get_current_pcb()->terminal_id]; i++)
  {
    buf[i] = terminal_buffer[get_current_pcb()->terminal_id][i];
    if(terminal_buffer[get_current_pcb()->terminal_id][i] == '\n')
    {
      i++;
      break;
    }
  }
  terminal_buffer_move(i);
  return i;
}


/* terminal_write
 * Write TO the screen from buff
 * Inputs: keyboard buffer, number of bytes
 * Outputs: number of bytes written or -1
 * Side Effects: none
 */
int terminal_write(int32_t fd, unsigned char* buf, int size)
{
    // Only Stout can be written to.
    if(fd != 1)
      return -1;

    if(buf==NULL || size < 0)
      return -1;

    int i;
    for(i = 0; i < size; i++)
    {
      // CAUTION: commented for test_terminal_write_size_larger_than_actual
      // if(buf[i] == '\0')
      //   break;
      putc(buf[i]);
    }
    return i;
}
