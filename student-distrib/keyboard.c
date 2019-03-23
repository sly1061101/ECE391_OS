/* keyboard.c - Functions to interact with the keyboard
 */

#include "keyboard.h"
#include "idt.h"

#define LOW_BIT_OFFSER 0x01

int caps_flag = 0;
int shift_flag = 0;
int alt_flag = 0;
int ctrl_flag = 0;
int default_flag = 0;
int terminal_flag = 0;
int backspace_flag = 0;

unsigned char keyboard_buffer[BUFFER_SIZE];
int keyboard_buffer_size;

unsigned char display_buff[MAP_SIZE];
/* keyboard_init
 * Initialized the keyboard
 * Inputs: None
 * Outputs: None
 * Side Effects: Enable irq 1
 */
void keyboard_init()
{
  keyboard_buffer_size = 0;
  interrupt_handler[KEYBOARD_IR_VEC] = keyboard_handler;
  enable_irq(KEYBOARD_IRQ);
}

// reference https://github.com/arjun024/mkeykernel
/* keyboard_handler
 * Keyboard interrupt handler with US keyboard layout
 * Inputs: None
 * Outputs: None
 * Side Effects: none
 */
void keyboard_handler()
{

  unsigned char status;
  char keycode;
  unsigned char keycode_processed;

  status = inb(KEYBOARD_STATUS_PORT);

  /* Lowest bit of status check empty */
  if (status & LOW_BIT_OFFSER)
  {
    keycode = inb(KEYBOARD_DATA_PORT);
    keycode_processed = char_converter((unsigned char)keycode);

    if (keycode >= 0)
    {

      if ((keycode_processed == 'l' || keycode_processed == 'L') && ctrl_flag ==1)
      {
        // First clear the screen, then print the content in keyboard_buffer so that the current line is preversed.
        clear();
        terminal_write(keyboard_buffer, keyboard_buffer_size);        
        send_eoi(KEYBOARD_IRQ);
        return;
      }

      if(backspace_flag) {
        if(keyboard_buffer_size > 0) {
          backspace_delete();
          keyboard_buffer_size--;
        }
      }

      //printf("%c",EMPTY);
      if(default_flag){
        if(keyboard_buffer_size < BUFFER_SIZE) {
          printf("%c", keycode_processed);
          keyboard_buffer[keyboard_buffer_size] = keycode_processed;
          keyboard_buffer_size++;

          if(keycode_processed == '\n') {
            // TODO: Store the current keyboard_buffer to terminal_buffer.
            keyboard_buffer_size = 0;
          }
        }
      }

    }

    send_eoi(KEYBOARD_IRQ);
  }
}

/* char_converter
 * Helper fcuntion to handle special input cases
 * Inputs: input -- unsigned char from KEYBOARD_DATA_PORT
 * Outputs: converted character 
 * Side Effects:s none
 */
char char_converter(unsigned char input)
{

  switch ((unsigned)input)
  {

  case CAPS_LOCK:
    caps_flag = PRESSED - caps_flag;
    default_flag = RELEASED;
    break;


  case LEFT_SHIFT:
  case RIGHT_SHIFT:
    shift_flag = PRESSED;
    default_flag = RELEASED;
    break;

  case LEFT_SHIFT + HIGH_ORDER_BIT_MASK:
  case RIGHT_SHIFT + HIGH_ORDER_BIT_MASK:
    shift_flag = RELEASED;
    default_flag = RELEASED;
    break;
    
  case CTRL:
    ctrl_flag = PRESSED;
    default_flag = RELEASED;
    break; 

  case CTRL + HIGH_ORDER_BIT_MASK:
    ctrl_flag = RELEASED;
    default_flag = RELEASED;
    break;

  case ALT:
    alt_flag = PRESSED;
    default_flag = RELEASED;
    break;

  case ALT + HIGH_ORDER_BIT_MASK:
    alt_flag = RELEASED;
    default_flag = RELEASED;
    break;

  case ENTER:
    terminal_flag = READY;
    default_flag = PRESSED;
    break;

  case BACKSPACE:
    backspace_flag = PRESSED;
    default_flag = RELEASED;
    break;

  case BACKSPACE + HIGH_ORDER_BIT_MASK:
    backspace_flag = RELEASED;
    default_flag = RELEASED;
    break;

  default:
    default_flag = PRESSED;
    break;
  }

  // if(!default_flag)
  //   return EMPTY;

  if (caps_flag)
  {
    if(shift_flag)
    {
      return keyboard_map_shift[(unsigned char)input];
    }
    return keyboard_map_caps[(unsigned char)input];
  }

  if (shift_flag)
  {
    return keyboard_map_shift[(unsigned char)input];
  }

  return keyboard_map[(unsigned char)input];
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
int terminal_close()

{
    return 0;
}

/* terminal_read
 * Read FROM the keyboard buffer into buf
 * Inputs: keyboard buffer
 * Outputs: number of bytes read
 * Side Effects: none
 */
int terminal_read(char* buf)
{
    if(terminal_flag){
      memcpy(buf,keyboard_buffer,sizeof(keyboard_buffer));
      // clear keyboard_buffer
      memset(keyboard_buffer,0,sizeof(keyboard_buffer));
      terminal_flag = 0;
      return sizeof(keyboard_buffer);
    }
    return -1; // is it necessary?
}


/* terminal_write
 * Write TO the screen from buff
 * Inputs: keyboard buffer, number of bytes
 * Outputs: number of bytes written or -1
 * Side Effects: none
 */
int terminal_write(char* buf, int size)

{
    memcpy(display_buff,buf,sizeof(display_buff));
    return size;
    //return -1;
}

