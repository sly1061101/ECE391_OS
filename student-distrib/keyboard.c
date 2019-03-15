/* keyboard.c - Functions to interact with the keyboard
 */

#include "keyboard.h"
#include "idt.h"

#define LOW_BIT_OFFSER 0x01

int rtc_test = 0;

int caps_flag = 0;
int shift_flag = 0;
int alt_flag = 0;
int ctrl_flag = 0;
/* keyboard_init
 * Initialized the keyboard
 * Inputs: None
 * Outputs: None
 * Side Effects: Enable irq 1
 */
void keyboard_init()
{
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

      if (keycode_processed == 'l' || keycode_processed == 'L')
      {
        clear(); // clear screen
        send_eoi(KEYBOARD_IRQ);
        return;
      }

      //printf("%c",keyboard_map[keycode]);
      printf("%c", keycode_processed);

      if (keyboard_map[(unsigned char)keycode] == 'r')
        rtc_test = 1;
      else
        rtc_test = 0;
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
    break;

  case LEFT_SHIFT:
  case RIGHT_SHIFT:
    shift_flag = PRESSED;
    break;

  // case LEFT_SHIFT + HIGH_ORDER_BIT_MASK:
  case LEFT_SHIFT_R:
 // case RIGHT_SHIFT + HIGH_ORDER_BIT_MASK:
  case RIGHT_SHIFT_R:
    shift_flag = RELEASED;
    break;

  case CTRL:
    ctrl_flag = PRESSED;
    break;

  case CTRL + HIGH_ORDER_BIT_MASK:
    ctrl_flag = RELEASED;
    break;

  case ALT:
    alt_flag = PRESSED;
    break;

  case ALT + HIGH_ORDER_BIT_MASK:
    alt_flag = RELEASED;
    break;

  default:
    break;
  }

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
