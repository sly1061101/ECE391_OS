/* lib.c - Some basic library functions (printf, strlen, etc.)
 * vim:ts=4 noexpandtab */

#include "lib.h"
#include "process.h"
#include "terminal.h"

// This virtual address is not always mapped to physical video memory.
//  When process on background terminal is running, it is mapped to the
//  backstorage space for that terminal.
#define VIDEO       0xB8000
// This virtual address is set up in all user program page
//  tables such that it is always mapped to physical video memory.
//  We can always use it to modify physical video memory (what is 
//  really shown on screen) no matter which process is running.
#define VIDEO_ALWAYS_TO_PHYSICAL 0xB9000

static void scroll();

static int screen_x;
static int screen_y;
static char* video_mem = (char *)VIDEO;
static char* video_mem_always_to_physical = (char *)(VIDEO_ALWAYS_TO_PHYSICAL);

// Load content from buf into video memory.
void load_video_memory(const char *buf) {
    memcpy(video_mem_always_to_physical, buf, 4 * 1024);
}

// Backup the content in video memory into buf.
void backup_video_memory(char *buf) {
    memcpy(buf, video_mem_always_to_physical, 4 * 1024);
}

// Load screen position.
void load_screen_position(int x, int y) {
    screen_x = x;
    screen_y = y;
}

// Backup screen position.
void backup_screen_position(int *x, int *y) {
    *x = screen_x;
    *y = screen_y;
}

/* void clear(void);
 * Inputs: void
 * Return Value: none
 * Function: Clears video memory */
void clear(void) {
    int32_t i;
    for (i = 0; i < NUM_ROWS * NUM_COLS; i++) {
        *(uint8_t *)(video_mem + (i << 1)) = ' ';
        *(uint8_t *)(video_mem + (i << 1) + 1) = ATTRIB;
    }
    screen_x = 0;
    screen_y = 0;
    if(get_current_pcb()->terminal_id == get_display_terminal())
        update_cursor(screen_x, screen_y);
}

/* void clear_display_terminal(void);
 * Inputs: void
 * Return Value: none
 * Function: Clears display terminal's video memory */
void clear_display_terminal(void) {
    if(get_current_pcb()->terminal_id == get_display_terminal()) {
        clear();
        return;
    }
    
    int32_t i;
    for (i = 0; i < NUM_ROWS * NUM_COLS; i++) {
        *(uint8_t *)(video_mem_always_to_physical + (i << 1)) = ' ';
        *(uint8_t *)(video_mem_always_to_physical + (i << 1) + 1) = ATTRIB;
    }

    screen_x_backstore[get_display_terminal()] = 0;
    screen_y_backstore[get_display_terminal()] = 0;
    update_cursor(screen_x_backstore[get_display_terminal()], screen_y_backstore[get_display_terminal()]);
}

// Constants that actually make no sense but just to
//  eliminate magic numbers.
#define PORT_0x3D4 0x3D4
#define PORT_0x3D5 0x3D5
#define ALL_MASK 0xFF
#define CMD_0x0E 0x0E
#define CMD_0x0F 0x0F
#define VAL_8 8

// Resource: https://wiki.osdev.org/Text_Mode_Cursor
/* update_cursor
 * Inputs: screen position for cursor
 * Return Value: none
 * Function: Update cursor position*/
void update_cursor(int x, int y)
{
	uint16_t pos = y * NUM_COLS + x;
 
	outb(CMD_0x0F, PORT_0x3D4);
	outb((uint8_t) (pos & ALL_MASK), PORT_0x3D5);
	outb(CMD_0x0E,PORT_0x3D4);
	outb((uint8_t) ((pos >> VAL_8) & ALL_MASK),PORT_0x3D5);
}

/*
*   scroll
*   Inputs: scroll screen
*   Return Value: void
*   Function: enable scrolling
*/
static void scroll()
{
    int x, y;
    int new_pos, old_pos;

    // For each row, we copy row x+1 to row x
    for(x = 0; x < NUM_ROWS - 1; x++)
    {
        for(y = 0; y < NUM_COLS; y++)
        {
            new_pos = NUM_COLS * x + y;
            old_pos = NUM_COLS * x + y + NUM_COLS;
            *(uint8_t *)(video_mem + (new_pos << 1)) = *(video_mem + (old_pos << 1));           
            *(uint8_t *)(video_mem + (new_pos << 1) + 1) = ATTRIB;
        }
    }
    
    x = NUM_ROWS-1;
    
    // clear last row
    for(y = 0; y < NUM_COLS; y++)
    {   
        new_pos = NUM_COLS * x + y;
        *(uint8_t *)(video_mem + (new_pos << 1)) = ' ';           
        *(uint8_t *)(video_mem + (new_pos << 1) + 1) = ATTRIB;
    }
    
    // set screen position to beginning of last row
    screen_x = 0;
    screen_y = NUM_ROWS - 1;
}

/*
*   scroll_display_terminal
*   Inputs: scroll display terminal's screen
*   Return Value: void
*   Function: enable scrolling
*/
static void scroll_display_terminal()
{
    if(get_current_pcb()->terminal_id == get_display_terminal()) {
        scroll();
        return;
    }

    int x, y;
    int new_pos, old_pos;

    // For each row, we copy row x+1 to row x
    for(x = 0; x < NUM_ROWS - 1; x++)
    {
        for(y = 0; y < NUM_COLS; y++)
        {
            new_pos = NUM_COLS * x + y;
            old_pos = NUM_COLS * x + y + NUM_COLS;
            *(uint8_t *)(video_mem_always_to_physical + (new_pos << 1)) = *(video_mem_always_to_physical + (old_pos << 1));           
            *(uint8_t *)(video_mem_always_to_physical + (new_pos << 1) + 1) = ATTRIB;
        }
    }
    
    x = NUM_ROWS-1;
    
    // clear last row
    for(y = 0; y < NUM_COLS; y++)
    {   
        new_pos = NUM_COLS * x + y;
        *(uint8_t *)(video_mem_always_to_physical + (new_pos << 1)) = ' ';           
        *(uint8_t *)(video_mem_always_to_physical + (new_pos << 1) + 1) = ATTRIB;
    }

    screen_x_backstore[get_display_terminal()] = 0;
    screen_y_backstore[get_display_terminal()] = NUM_ROWS - 1;
    update_cursor(screen_x_backstore[get_display_terminal()], screen_y_backstore[get_display_terminal()]);
}

/*
*   backspace_delete
*   Inputs: none
*   Return Value: void
*   Function:  perform backspace
*/
void backspace_delete() {
    // If already at beginning of first line, do nothing.
    if(screen_x == 0 && screen_y == 0) {
        return;
    }
    // If at beginning of not first line, go back to previous line.
    else if (screen_x == 0 && screen_y > 0) {
        screen_x = NUM_COLS - 1;     
        screen_y--;
    }
    // Otherwise just go backware.
    else {
        screen_x--;         
    }                    

    // change registers 
    *(uint8_t *)(video_mem+ ((NUM_COLS * screen_y + screen_x) << 1)) = ' ';
    *(uint8_t *)(video_mem + ((NUM_COLS * screen_y + screen_x) << 1) + 1) = ATTRIB;  
    
    //update the cursor
    if(get_current_pcb()->terminal_id == get_display_terminal())
        update_cursor(screen_x, screen_y);                                                                                          
}

/*
*   backspace_delete_display_terminal
*   Inputs: none
*   Return Value: void
*   Function:  perform backspace on display terminal
*/

void backspace_delete_display_terminal() {
    if(get_current_pcb()->terminal_id == get_display_terminal()) {
        backspace_delete();
        return;
    }

    // If already at beginning of first line, do nothing.
    if(screen_x_backstore[get_display_terminal()] == 0 && screen_y_backstore[get_display_terminal()] == 0) {
        return;
    }
    // If at beginning of not first line, go back to previous line.
    else if (screen_x_backstore[get_display_terminal()] == 0 && screen_y_backstore[get_display_terminal()] > 0) {
        screen_x_backstore[get_display_terminal()] = NUM_COLS - 1;     
        screen_y_backstore[get_display_terminal()]--;
    }
    // Otherwise just go backware.
    else {
        screen_x_backstore[get_display_terminal()]--;         
    }                    

    // change registers 
    *(uint8_t *)(video_mem_always_to_physical + ((NUM_COLS * screen_y_backstore[get_display_terminal()] + screen_x_backstore[get_display_terminal()]) << 1)) = ' ';
    *(uint8_t *)(video_mem_always_to_physical + ((NUM_COLS * screen_y_backstore[get_display_terminal()] + screen_x_backstore[get_display_terminal()]) << 1) + 1) = ATTRIB;  
    
    update_cursor(screen_x_backstore[get_display_terminal()], screen_y_backstore[get_display_terminal()]);                                                                                            
}

/* Standard printf().
 * Only supports the following format strings:
 * %%  - print a literal '%' character
 * %x  - print a number in hexadecimal
 * %u  - print a number as an unsigned integer
 * %d  - print a number as a signed integer
 * %c  - print a character
 * %s  - print a string
 * %#x - print a number in 32-bit aligned hexadecimal, i.e.
 *       print 8 hexadecimal digits, zero-padded on the left.
 *       For example, the hex number "E" would be printed as
 *       "0000000E".
 *       Note: This is slightly different than the libc specification
 *       for the "#" modifier (this implementation doesn't add a "0x" at
 *       the beginning), but I think it's more flexible this way.
 *       Also note: %x is the only conversion specifier that can use
 *       the "#" modifier to alter output. */
int32_t printf(int8_t *format, ...) {

    /* Pointer to the format string */
    int8_t* buf = format;

    /* Stack pointer for the other parameters */
    int32_t* esp = (void *)&format;
    esp++;

    while (*buf != '\0') {
        switch (*buf) {
            case '%':
                {
                    int32_t alternate = 0;
                    buf++;

format_char_switch:
                    /* Conversion specifiers */
                    switch (*buf) {
                        /* Print a literal '%' character */
                        case '%':
                            putc('%');
                            break;

                        /* Use alternate formatting */
                        case '#':
                            alternate = 1;
                            buf++;
                            /* Yes, I know gotos are bad.  This is the
                             * most elegant and general way to do this,
                             * IMHO. */
                            goto format_char_switch;

                        /* Print a number in hexadecimal form */
                        case 'x':
                            {
                                int8_t conv_buf[64];
                                if (alternate == 0) {
                                    itoa(*((uint32_t *)esp), conv_buf, 16);
                                    puts(conv_buf);
                                } else {
                                    int32_t starting_index;
                                    int32_t i;
                                    itoa(*((uint32_t *)esp), &conv_buf[8], 16);
                                    i = starting_index = strlen(&conv_buf[8]);
                                    while(i < 8) {
                                        conv_buf[i] = '0';
                                        i++;
                                    }
                                    puts(&conv_buf[starting_index]);
                                }
                                esp++;
                            }
                            break;

                        /* Print a number in unsigned int form */
                        case 'u':
                            {
                                int8_t conv_buf[36];
                                itoa(*((uint32_t *)esp), conv_buf, 10);
                                puts(conv_buf);
                                esp++;
                            }
                            break;

                        /* Print a number in signed int form */
                        case 'd':
                            {
                                int8_t conv_buf[36];
                                int32_t value = *((int32_t *)esp);
                                if(value < 0) {
                                    conv_buf[0] = '-';
                                    itoa(-value, &conv_buf[1], 10);
                                } else {
                                    itoa(value, conv_buf, 10);
                                }
                                puts(conv_buf);
                                esp++;
                            }
                            break;

                        /* Print a single character */
                        case 'c':
                            putc((uint8_t) *((int32_t *)esp));
                            esp++;
                            break;

                        /* Print a NULL-terminated string */
                        case 's':
                            puts(*((int8_t **)esp));
                            esp++;
                            break;

                        default:
                            break;
                    }

                }
                break;

            default:
                putc(*buf);
                break;
        }
        buf++;
    }
    return (buf - format);
}

/*
*   printf_display_terminal
*   Inputs: format
*   Return Value: buf
*   Function: standard printf on display temrinal
*/
int32_t printf_display_terminal(int8_t *format, ...) {

    /* Pointer to the format string */
    int8_t* buf = format;

    /* Stack pointer for the other parameters */
    int32_t* esp = (void *)&format;
    esp++;

    while (*buf != '\0') {
        switch (*buf) {
            case '%':
                {
                    int32_t alternate = 0;
                    buf++;

format_char_switch:
                    /* Conversion specifiers */
                    switch (*buf) {
                        /* Print a literal '%' character */
                        case '%':
                            putc_display_terminal('%');
                            break;

                        /* Use alternate formatting */
                        case '#':
                            alternate = 1;
                            buf++;
                            /* Yes, I know gotos are bad.  This is the
                             * most elegant and general way to do this,
                             * IMHO. */
                            goto format_char_switch;

                        /* Print a number in hexadecimal form */
                        case 'x':
                            {
                                int8_t conv_buf[64];
                                if (alternate == 0) {
                                    itoa(*((uint32_t *)esp), conv_buf, 16);
                                    puts_display_terminal(conv_buf);
                                } else {
                                    int32_t starting_index;
                                    int32_t i;
                                    itoa(*((uint32_t *)esp), &conv_buf[8], 16);
                                    i = starting_index = strlen(&conv_buf[8]);
                                    while(i < 8) {
                                        conv_buf[i] = '0';
                                        i++;
                                    }
                                    puts_display_terminal(&conv_buf[starting_index]);
                                }
                                esp++;
                            }
                            break;

                        /* Print a number in unsigned int form */
                        case 'u':
                            {
                                int8_t conv_buf[36];
                                itoa(*((uint32_t *)esp), conv_buf, 10);
                                puts_display_terminal(conv_buf);
                                esp++;
                            }
                            break;

                        /* Print a number in signed int form */
                        case 'd':
                            {
                                int8_t conv_buf[36];
                                int32_t value = *((int32_t *)esp);
                                if(value < 0) {
                                    conv_buf[0] = '-';
                                    itoa(-value, &conv_buf[1], 10);
                                } else {
                                    itoa(value, conv_buf, 10);
                                }
                                puts_display_terminal(conv_buf);
                                esp++;
                            }
                            break;

                        /* Print a single character */
                        case 'c':
                            putc_display_terminal((uint8_t) *((int32_t *)esp));
                            esp++;
                            break;

                        /* Print a NULL-terminated string */
                        case 's':
                            puts_display_terminal(*((int8_t **)esp));
                            esp++;
                            break;

                        default:
                            break;
                    }

                }
                break;

            default:
                putc_display_terminal(*buf);
                break;
        }
        buf++;
    }
    return (buf - format);
}

/* int32_t puts(int8_t* s);
 *   Inputs: int_8* s = pointer to a string of characters
 *   Return Value: Number of bytes written
 *    Function: Output a string to the console */
int32_t puts(int8_t* s) {
    register int32_t index = 0;
    while (s[index] != '\0') {
        putc(s[index]);
        index++;
    }
    return index;
}

/* int32_t puts_display_terminal;
 *   Inputs: int_8* s = pointer to a string of characters
 *   Return Value: Number of bytes written
 *   Function: Output a string to the console on display terminal */
int32_t puts_display_terminal(int8_t* s) {
    if(get_current_pcb()->terminal_id == get_display_terminal()) {
        return puts(s);
    }
    
    register int32_t index = 0;
    while (s[index] != '\0') {
        putc_display_terminal(s[index]);
        index++;
    }
    return index;
}

/* void putc(uint8_t c);
 * Inputs: uint_8* c = character to print
 * Return Value: void
 *  Function: Output a character to the console */
void putc(uint8_t c) {
    if(c == '\n' || c == '\r') {
        screen_x = 0;
        screen_y++;
    } else {
        *(uint8_t *)(video_mem + ((NUM_COLS * screen_y + screen_x) << 1)) = c;
        *(uint8_t *)(video_mem + ((NUM_COLS * screen_y + screen_x) << 1) + 1) = ATTRIB;
        screen_x++;
        if(screen_x == NUM_COLS) {
            screen_x = 0;
            screen_y++;
        }
    }
    if(screen_y == NUM_ROWS)
        scroll();
    if(get_current_pcb()->terminal_id == get_display_terminal())
        update_cursor(screen_x, screen_y);
}

/* void putc_display_terminal(uint8_t c);
 * Inputs: uint_8* c = character to print
 * Return Value: void
 *  Function: Output a character to the console on display terminal*/
void putc_display_terminal(uint8_t c) {
    if(get_current_pcb()->terminal_id == get_display_terminal()) {
        putc(c);
        return;
    }

    if(c == '\n' || c == '\r') {
        screen_x_backstore[get_display_terminal()] = 0;
        screen_y_backstore[get_display_terminal()]++;
    } else {
        *(uint8_t *)(video_mem_always_to_physical + ((NUM_COLS * screen_y_backstore[get_display_terminal()] + screen_x_backstore[get_display_terminal()]) << 1)) = c;
        *(uint8_t *)(video_mem_always_to_physical + ((NUM_COLS * screen_y_backstore[get_display_terminal()] + screen_x_backstore[get_display_terminal()]) << 1) + 1) = ATTRIB;
        screen_x_backstore[get_display_terminal()]++;
        if(screen_x_backstore[get_display_terminal()] == NUM_COLS) {
            screen_x_backstore[get_display_terminal()] = 0;
            screen_y_backstore[get_display_terminal()]++;
        }
    }
    if(screen_y_backstore[get_display_terminal()] == NUM_ROWS)
        scroll_display_terminal();

    update_cursor(screen_x_backstore[get_display_terminal()], screen_y_backstore[get_display_terminal()]);        
}

/* int8_t* itoa(uint32_t value, int8_t* buf, int32_t radix);
 * Inputs: uint32_t value = number to convert
 *            int8_t* buf = allocated buffer to place string in
 *          int32_t radix = base system. hex, oct, dec, etc.
 * Return Value: number of bytes written
 * Function: Convert a number to its ASCII representation, with base "radix" */
int8_t* itoa(uint32_t value, int8_t* buf, int32_t radix) {
    static int8_t lookup[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    int8_t *newbuf = buf;
    int32_t i;
    uint32_t newval = value;

    /* Special case for zero */
    if (value == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return buf;
    }

    /* Go through the number one place value at a time, and add the
     * correct digit to "newbuf".  We actually add characters to the
     * ASCII string from lowest place value to highest, which is the
     * opposite of how the number should be printed.  We'll reverse the
     * characters later. */
    while (newval > 0) {
        i = newval % radix;
        *newbuf = lookup[i];
        newbuf++;
        newval /= radix;
    }

    /* Add a terminating NULL */
    *newbuf = '\0';

    /* Reverse the string and return */
    return strrev(buf);
}

/* int8_t* strrev(int8_t* s);
 * Inputs: int8_t* s = string to reverse
 * Return Value: reversed string
 * Function: reverses a string s */
int8_t* strrev(int8_t* s) {
    register int8_t tmp;
    register int32_t beg = 0;
    register int32_t end = strlen(s) - 1;

    while (beg < end) {
        tmp = s[end];
        s[end] = s[beg];
        s[beg] = tmp;
        beg++;
        end--;
    }
    return s;
}

/* uint32_t strlen(const int8_t* s);
 * Inputs: const int8_t* s = string to take length of
 * Return Value: length of string s
 * Function: return length of string s */
uint32_t strlen(const int8_t* s) {
    register uint32_t len = 0;
    while (s[len] != '\0')
        len++;
    return len;
}

/* void* memset(void* s, int32_t c, uint32_t n);
 * Inputs:    void* s = pointer to memory
 *          int32_t c = value to set memory to
 *         uint32_t n = number of bytes to set
 * Return Value: new string
 * Function: set n consecutive bytes of pointer s to value c */
void* memset(void* s, int32_t c, uint32_t n) {
    c &= 0xFF;
    asm volatile ("                 \n\
            .memset_top:            \n\
            testl   %%ecx, %%ecx    \n\
            jz      .memset_done    \n\
            testl   $0x3, %%edi     \n\
            jz      .memset_aligned \n\
            movb    %%al, (%%edi)   \n\
            addl    $1, %%edi       \n\
            subl    $1, %%ecx       \n\
            jmp     .memset_top     \n\
            .memset_aligned:        \n\
            movw    %%ds, %%dx      \n\
            movw    %%dx, %%es      \n\
            movl    %%ecx, %%edx    \n\
            shrl    $2, %%ecx       \n\
            andl    $0x3, %%edx     \n\
            cld                     \n\
            rep     stosl           \n\
            .memset_bottom:         \n\
            testl   %%edx, %%edx    \n\
            jz      .memset_done    \n\
            movb    %%al, (%%edi)   \n\
            addl    $1, %%edi       \n\
            subl    $1, %%edx       \n\
            jmp     .memset_bottom  \n\
            .memset_done:           \n\
            "
            :
            : "a"(c << 24 | c << 16 | c << 8 | c), "D"(s), "c"(n)
            : "edx", "memory", "cc"
    );
    return s;
}

/* void* memset_word(void* s, int32_t c, uint32_t n);
 * Description: Optimized memset_word
 * Inputs:    void* s = pointer to memory
 *          int32_t c = value to set memory to
 *         uint32_t n = number of bytes to set
 * Return Value: new string
 * Function: set lower 16 bits of n consecutive memory locations of pointer s to value c */
void* memset_word(void* s, int32_t c, uint32_t n) {
    asm volatile ("                 \n\
            movw    %%ds, %%dx      \n\
            movw    %%dx, %%es      \n\
            cld                     \n\
            rep     stosw           \n\
            "
            :
            : "a"(c), "D"(s), "c"(n)
            : "edx", "memory", "cc"
    );
    return s;
}

/* void* memset_dword(void* s, int32_t c, uint32_t n);
 * Inputs:    void* s = pointer to memory
 *          int32_t c = value to set memory to
 *         uint32_t n = number of bytes to set
 * Return Value: new string
 * Function: set n consecutive memory locations of pointer s to value c */
void* memset_dword(void* s, int32_t c, uint32_t n) {
    asm volatile ("                 \n\
            movw    %%ds, %%dx      \n\
            movw    %%dx, %%es      \n\
            cld                     \n\
            rep     stosl           \n\
            "
            :
            : "a"(c), "D"(s), "c"(n)
            : "edx", "memory", "cc"
    );
    return s;
}

/* void* memcpy(void* dest, const void* src, uint32_t n);
 * Inputs:      void* dest = destination of copy
 *         const void* src = source of copy
 *              uint32_t n = number of byets to copy
 * Return Value: pointer to dest
 * Function: copy n bytes of src to dest */
void* memcpy(void* dest, const void* src, uint32_t n) {
    asm volatile ("                 \n\
            .memcpy_top:            \n\
            testl   %%ecx, %%ecx    \n\
            jz      .memcpy_done    \n\
            testl   $0x3, %%edi     \n\
            jz      .memcpy_aligned \n\
            movb    (%%esi), %%al   \n\
            movb    %%al, (%%edi)   \n\
            addl    $1, %%edi       \n\
            addl    $1, %%esi       \n\
            subl    $1, %%ecx       \n\
            jmp     .memcpy_top     \n\
            .memcpy_aligned:        \n\
            movw    %%ds, %%dx      \n\
            movw    %%dx, %%es      \n\
            movl    %%ecx, %%edx    \n\
            shrl    $2, %%ecx       \n\
            andl    $0x3, %%edx     \n\
            cld                     \n\
            rep     movsl           \n\
            .memcpy_bottom:         \n\
            testl   %%edx, %%edx    \n\
            jz      .memcpy_done    \n\
            movb    (%%esi), %%al   \n\
            movb    %%al, (%%edi)   \n\
            addl    $1, %%edi       \n\
            addl    $1, %%esi       \n\
            subl    $1, %%edx       \n\
            jmp     .memcpy_bottom  \n\
            .memcpy_done:           \n\
            "
            :
            : "S"(src), "D"(dest), "c"(n)
            : "eax", "edx", "memory", "cc"
    );
    return dest;
}

/* void* memmove(void* dest, const void* src, uint32_t n);
 * Description: Optimized memmove (used for overlapping memory areas)
 * Inputs:      void* dest = destination of move
 *         const void* src = source of move
 *              uint32_t n = number of byets to move
 * Return Value: pointer to dest
 * Function: move n bytes of src to dest */
void* memmove(void* dest, const void* src, uint32_t n) {
    asm volatile ("                             \n\
            movw    %%ds, %%dx                  \n\
            movw    %%dx, %%es                  \n\
            cld                                 \n\
            cmp     %%edi, %%esi                \n\
            jae     .memmove_go                 \n\
            leal    -1(%%esi, %%ecx), %%esi     \n\
            leal    -1(%%edi, %%ecx), %%edi     \n\
            std                                 \n\
            .memmove_go:                        \n\
            rep     movsb                       \n\
            "
            :
            : "D"(dest), "S"(src), "c"(n)
            : "edx", "memory", "cc"
    );
    return dest;
}

/* int32_t strncmp(const int8_t* s1, const int8_t* s2, uint32_t n)
 * Inputs: const int8_t* s1 = first string to compare
 *         const int8_t* s2 = second string to compare
 *               uint32_t n = number of bytes to compare
 * Return Value: A zero value indicates that the characters compared
 *               in both strings form the same string.
 *               A value greater than zero indicates that the first
 *               character that does not match has a greater value
 *               in str1 than in str2; And a value less than zero
 *               indicates the opposite.
 * Function: compares string 1 and string 2 for equality */
int32_t strncmp(const int8_t* s1, const int8_t* s2, uint32_t n) {
    int32_t i;
    for (i = 0; i < n; i++) {
        if ((s1[i] != s2[i]) || (s1[i] == '\0') /* || s2[i] == '\0' */) {

            /* The s2[i] == '\0' is unnecessary because of the short-circuit
             * semantics of 'if' expressions in C.  If the first expression
             * (s1[i] != s2[i]) evaluates to false, that is, if s1[i] ==
             * s2[i], then we only need to test either s1[i] or s2[i] for
             * '\0', since we know they are equal. */
            return s1[i] - s2[i];
        }
    }
    return 0;
}

/* int8_t* strcpy(int8_t* dest, const int8_t* src)
 * Inputs:      int8_t* dest = destination string of copy
 *         const int8_t* src = source string of copy
 * Return Value: pointer to dest
 * Function: copy the source string into the destination string */
int8_t* strcpy(int8_t* dest, const int8_t* src) {
    int32_t i = 0;
    while (src[i] != '\0') {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
    return dest;
}

/* int8_t* strcpy(int8_t* dest, const int8_t* src, uint32_t n)
 * Inputs:      int8_t* dest = destination string of copy
 *         const int8_t* src = source string of copy
 *                uint32_t n = number of bytes to copy
 * Return Value: pointer to dest
 * Function: copy n bytes of the source string into the destination string */
int8_t* strncpy(int8_t* dest, const int8_t* src, uint32_t n) {
    int32_t i = 0;
    while (src[i] != '\0' && i < n) {
        dest[i] = src[i];
        i++;
    }
    while (i < n) {
        dest[i] = '\0';
        i++;
    }
    return dest;
}

/* void test_interrupts(void)
 * Inputs: void
 * Return Value: void
 * Function: increments video memory. To be used to test rtc */
void test_interrupts(void) {
    int32_t i;
    for (i = 0; i < NUM_ROWS * NUM_COLS; i++) {
        video_mem[i << 1]++;
    }
}
