/* keyboard.c - Functions to interact with the keyboard
 */

#include "keyboard.h"
#include "idt.h"
#include "terminal.h"
#include "keyboard_scancode.h"
#include "process.h"

// special flags 
int caps_flag = 0;
int shift_flag = 0;
int alt_flag = 0;
int ctrl_flag = 0;
int default_flag = 0;
int backspace_flag = 0;
int esc_flag = 0;

unsigned char keyboard_buffer[TERMINAL_NUM][KEYBOARD_BUFFER_CAPACITY];
int keyboard_buffer_size[TERMINAL_NUM];

/* keyboard_init
 * Initialized the keyboard
 * Inputs: None
 * Outputs: None
 * Side Effects: Enable irq 1
 */
void keyboard_init()
{
  int i;
  for (i = 0 ; i < TERMINAL_NUM; i++)
    keyboard_buffer_size[i] = 0;

  interrupt_handler[KEYBOARD_VEC_NUM] = keyboard_handler;
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

    if(alt_flag && (keycode == F1 || keycode == F2 || keycode == F3)){
      switch (keycode)
      {
        case F1:
          switch_terminal(0);
          break;

        case F2:
          switch_terminal(1);
          break;

        case F3:
          switch_terminal(2);
          break;
        
        default:
          printf("Should never reach here!\n");
          break;
      }
      send_eoi(KEYBOARD_IRQ);
      return;
    }

    if (keycode >= 0)
    {
      // CAUTION: Everything here must be showing on the displayed terminal instead of the currently running terminal. 
      //  Set of functions have been created in lib.h/c to help. E.g. prinf_display_terminal(), putc_display_terminal() etc.
      int display_terminal = get_display_terminal();

      if ((keycode_processed == 'l' || keycode_processed == 'L') && ctrl_flag ==1)
      {
        // First clear the screen, then print the content in keyboard_buffer so that the current line is preversed.
        clear_display_terminal();
        printf_display_terminal("391OS> ");
        terminal_write_display_terminal(1, keyboard_buffer[display_terminal], keyboard_buffer_size[display_terminal]);        
        send_eoi(KEYBOARD_IRQ);
        return;
      }

      if(backspace_flag) {
        if(keyboard_buffer_size[display_terminal] > 0) {
          backspace_delete_display_terminal();
          keyboard_buffer_size[display_terminal]--;
        }
      }
      
      // print known default scancode
      if(default_flag){
        if(keyboard_buffer_size[display_terminal] < KEYBOARD_BUFFER_CAPACITY) {
          if(keycode_processed !=0 && keycode_processed != '\t'){
            printf_display_terminal("%c", keycode_processed);
            int temp = keyboard_buffer_size[display_terminal];
            keyboard_buffer[display_terminal][temp] = keycode_processed;
            keyboard_buffer_size[display_terminal]++;

            // terminal buffer write when ENTER is pressed
            if(keycode_processed == '\n') {
              terminal_buffer_write(keyboard_buffer[display_terminal], keyboard_buffer_size[display_terminal]);
              keyboard_buffer_size[display_terminal] = 0;
            }
          }
        }
        // Special case when already has 128 characters and enter is pressed.
        else if(keyboard_buffer_size[display_terminal] == KEYBOARD_BUFFER_CAPACITY 
                  && keycode_processed == '\n') {
          printf_display_terminal("%c", keycode_processed);
          terminal_buffer_write(keyboard_buffer[display_terminal], keyboard_buffer_size[display_terminal]);
          keyboard_buffer_size[display_terminal] = 0;
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
 * Side Effects: none
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

    case BACKSPACE:
      backspace_flag = PRESSED;
      default_flag = RELEASED;
      break;

    case BACKSPACE + HIGH_ORDER_BIT_MASK:
      backspace_flag = RELEASED;
      default_flag = RELEASED;
      break;

    case ESC:
      esc_flag = PRESSED;
      default_flag = RELEASED;
      break;
    
    case ESC + HIGH_ORDER_BIT_MASK:
      esc_flag = RELEASED;
      default_flag = RELEASED;
      break;
    
    default:
      default_flag = PRESSED;
      break;
  }

  // CAPSLOCK and SHIFT corner case
  if (caps_flag)
  {
    if(shift_flag)
    {
      return keyboard_map_shift_lower[(unsigned char)input];
    }
    return keyboard_map_caps[(unsigned char)input];
  }

  if (shift_flag)
  {
    return keyboard_map_shift_upper[(unsigned char)input];
  }

  return keyboard_map[(unsigned char)input];
}
