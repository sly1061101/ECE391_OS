/* keyboard.c - Functions to interact with the keyboard
 */

#include "keyboard.h"
#include "idt.h"


#define LOW_BIT_OFFSER 0x01

/* keyboard_init
 * Initialized the keyboard
 * Inputs: None
 * Outputs: None
 * Side Effects: Enable irq 1
 */
void keyboard_init(){
    interrupt_handler[KEYBOARD_IR_VEC] = keyboard_handler;
    enable_irq(KEYBOARD_IRQ);

}

int rtc_test = 0;

// reference https://github.com/arjun024/mkeykernel
/* keyboard_handler
 * Keyboard interrupt handler with US keyboard layout
 * Inputs: None
 * Outputs: None
 * Side Effects: none
 */
void keyboard_handler(){

    unsigned char status;
    char keycode;
    unsigned char keycode_processed;

    status = inb(KEYBOARD_STATUS_PORT);

  /* Lowest bit of status check empty */
      if (status & LOW_BIT_OFFSER) {
		keycode = inb(KEYBOARD_DATA_PORT);
    keycode_processed = char_converter(keycode);
		  if(keycode >= 0){
        
        //printf("%c",keyboard_map[keycode]);
        printf("%c",keycode_processed);

        if(keyboard_map[(unsigned char)keycode] == 'r')
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
char char_converter(char input){

    // char input_test = keyboard_map[input];
    // if(input_test==TAB){
    //   return keyboard_map_caps[input_test];
    // }
     return keyboard_map[input];
    
} 
