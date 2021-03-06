#include "terminal.h"
#include "paging.h"
#include "lib.h"

unsigned char terminal_buffer[TERMINAL_NUM][TERMINAL_BUFFER_CAPACITY];
int terminal_buffer_size[TERMINAL_NUM];

// Must align to 4KB boundary since it would be used in page table.
uint8_t video_mem_backstore[TERMINAL_NUM][VAL_4 * VAL_1024] __attribute__((aligned(VAL_4096)));

int screen_x_backstore[TERMINAL_NUM];
int screen_y_backstore[TERMINAL_NUM];

int display_terminal;


/* get_display_terminal
* get the display terminal by return the display terminal address
* Input: None
* Output: None
* Side Effects: none
*/
int get_display_terminal() {
    return display_terminal;
}

// 0 inactive, 1 active.
int32_t terminal_state[TERMINAL_NUM];

/* set_terminal_state
* set the terminal state
* Input: terminal id and state
* Output: None
* Side Effects: none
*/
void set_terminal_state(uint32_t terminal_id, uint32_t state) {
  terminal_state[terminal_id] = state;
}

/* get_next_inactive_terminal
* get the next inactive terminal
* Input: None
* Output: i for sucess and 0 for fail
* Side Effects: none
*/

extern int32_t get_next_inactive_terminal() {
  int32_t i;
  for(i = 0; i < TERMINAL_NUM; ++i) {
    if(terminal_state[i] == TERMINAL_INACTIVE)
      return i;
  }
  return -1;
}

/* terminal_init
* initilize the terminal
* Input: None
* Output: none
* Side Effects: none
*/


void terminal_init() {
  int i;
  for (i = 0 ; i < TERMINAL_NUM; i++) {
    terminal_buffer_size[i] = 0;
    terminal_state[i] = TERMINAL_INACTIVE;
    screen_x_backstore[i] = 0;
    screen_y_backstore[i] = 0;

    int32_t j;
    for (j = 0; j < NUM_COLS * NUM_ROWS; j++) {
        *(uint8_t *)(video_mem_backstore[i] + (j << 1)) = ' ';
        *(uint8_t *)(video_mem_backstore[i] + (j << 1) + 1) = ATTRIB;
    }
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


/* switch_terminal
* Switch the terminal based on the terminal id 
* Input: terminal id
* Output: none
* Side Effects: none
*/
void switch_terminal(uint32_t terminal_id) {
  
  backup_video_memory((char *)(video_mem_backstore[display_terminal]));
  load_video_memory((char *)(video_mem_backstore[terminal_id]));

  page_table_terminal_video_memory[display_terminal][VAL_184].page_base_address = (uint32_t)video_mem_backstore[display_terminal] >> 12;
  page_table_terminal_video_memory[terminal_id][VAL_184].page_base_address = VIDEO >> 12;

  page_table_program_vidmap[display_terminal][VAL_512].page_base_address = (uint32_t)video_mem_backstore[display_terminal] >> 12;
  page_table_program_vidmap[terminal_id][VAL_512].page_base_address = VIDEO >> 12;

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

/* terminal_write_display_terminal
 * Write TO the display terminal
 * Inputs: file directory, buffer and the size
 * Outputs: number of bytes written or -1
 * Side Effects: none
 */
int terminal_write_display_terminal(int32_t fd, unsigned char* buf, int size)
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
      putc_display_terminal(buf[i]);
    }
    return i;
}
