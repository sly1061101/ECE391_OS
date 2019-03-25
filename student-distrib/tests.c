#include "tests.h"
#include "x86_desc.h"
#include "lib.h"
#include "paging.h"
#include "file_system.h"
#include "keyboard.h"
#include "rtc.h"

#define PASS 1
#define FAIL 0
#define KB_IDT 0x21
#define RTC_IDT 0x28
#define N_BYTES_INT 4
#define VAL_INVALID 888
#define RTC_TEST_COUNT 30
#define STRING_SIZE 10
#define LARGER_STRING_SIZE (STRING_SIZE + 10)
#define FILE_READ_BUF_SIZE 10000

// Constants that actually make no sense but just to
//  eliminate magic numbers.
#define VAL_10 10
#define VAL_5 5
#define VAL_184 184

// global variable defined in rtc.c that increment per rtc interrupt handler
extern int rtc_counter;

/* format these macros as you see fit */
#define TEST_HEADER 	\
	printf("[TEST %s] Running %s at %s:%d\n", __FUNCTION__, __FUNCTION__, __FILE__, __LINE__)
#define TEST_OUTPUT(name, result)	\
	printf("[TEST %s] Result = %s\n", name, (result) ? "PASS" : "FAIL");

static inline void assertion_failure(){
	/* Use exception #15 for assertions, otherwise
	   reserved by Intel */
	asm volatile("int $15");
}


/* Checkpoint 1 tests */

/* IDT Test - Example
 * 
 * Asserts that first 10 IDT entries are not NULL
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: Load IDT, IDT definition
 * Files: x86_desc.h/S
 */
int idt_test(){
	TEST_HEADER;

	int i;
	int result = PASS;
	for (i = 0; i < VAL_10; ++i){
		if ((idt[i].offset_15_00 == NULL) && 
			(idt[i].offset_31_16 == NULL)){
			assertion_failure();
			result = FAIL;
		}		
	}

	if ((idt[RTC_IDT].offset_15_00 == NULL) && 
			(idt[RTC_IDT].offset_31_16 == NULL)){
			assertion_failure();
			result = FAIL;
		}

    if ((idt[RTC_IDT].offset_15_00 == NULL) && 
			(idt[RTC_IDT].offset_31_16 == NULL)){
			assertion_failure();
			result = FAIL;
		}

	return result;
}

// add more tests here

/* exception_de_test
*
* Trigger a exception 0 : divide by zero
* Inputs: None
* Outputs: PASS/FAIL
* Coverage: A piece of exception that should be handled
* Files: idt.c
*/
int exception_de_test(){
    TEST_HEADER;
    int a = VAL_5;
    int b = 0;
    int c;
	c = a/b;

	return PASS;
}

/* general_exception_test
*
* Trigger a general exception
* Inputs: None
* Outputs: PASS/FAIL
* Coverage: Test whether IDT is initialized correctly and assembly 
*			linkage and exception handlers are working.
* Files: idt.c, interrupt_linkage.S
*/
int general_exception_test(){
    TEST_HEADER;

	// Replace the interrupt number to what you want to trigger.
	asm volatile("int $10");

	return PASS;
}

/* deref_valid_addresses
*
* Dereference some valid memory addresses which have been mapped into physical memory.
* Inputs: None
* Outputs: PASS/FAIL
* Coverage: Test whether paging, PD and PT are initialized correctly.
* Files: paging.S
*/
int deref_valid_addresses(){
    TEST_HEADER;

	int result = PASS;

	uint8_t *p;

	// Video memory page.
	p = (uint8_t *)0x000B8000;
	printf("The byte stored at address 0x%#x is 0x%x.\n", p, *p);

	// Kernel memory page.
	p = (uint8_t *)0x00400000;
	printf("The byte stored at address 0x%#x is 0x%x.\n", p, *p);
	p = (uint8_t *)0x00500000;
	printf("The byte stored at address 0x%#x is 0x%x.\n", p, *p);
	p = (uint8_t *)0x00600000;
	printf("The byte stored at address 0x%#x is 0x%x.\n", p, *p);
	p = (uint8_t *)0x00700000;
	printf("The byte stored at address 0x%#x is 0x%x.\n", p, *p);
	p = (uint8_t *)0x007FFFFF;
	printf("The byte stored at address 0x%#x is 0x%x.\n", p, *p);

    return result;
}

/* deref_invalid_address
*
* Dereference invalid memory address which hasn't been mapped into physical memory.
* Inputs: None
* Outputs: PASS/FAIL
* Coverage: Test whether paging, PD and PT are initialized correctly.
* Files: paging.S
*/
int deref_invalid_address(){
    TEST_HEADER;

	uint8_t *p;

	// Set this to be an invalid address.
	p = (uint8_t *)0x000A0000;
	printf("The byte stored at address 0x%#x is 0x%x.\n", p, *p);

	return PASS;
}

/* deref_null_address
*
* Dereference null memory address.
* Inputs: None
* Outputs: PASS/FAIL
* Coverage: Test whether IDT is initialized correctly and assembly 
			linkage and exception handlers are working.Test whether
			paging, PD and PT are initialized correctly.
* Files: paging.S
*/
int deref_null_address(){
    TEST_HEADER;

	uint8_t *p = NULL;
	printf("The byte stored at address 0x%#x is 0x%x.\n", p, *p);

	return PASS;
}

/* pdt_and_pt_test
*
* Test values in page directory table and page table.
* Inputs: None
* Outputs: PASS/FAIL
* Coverage: Test whether values in page directory table
			and page table are correct.
* Files: paging.S
*/
int pdt_and_pt_test(){
    TEST_HEADER;

	int i;
	int result = PASS;

	// Test page directory table.
	for(i = 0; i < NUM_PDT_SIZE; ++i) {
		if(i == 0) {
			if(page_directory_table[i].entry_PT.present != 1
			|| page_directory_table[i].entry_PT.page_size != 0) {
				assertion_failure();
				result = FAIL;
			}
		}
		else if(i == 1) {
			if(page_directory_table[i].entry_PT.present != 1
			|| page_directory_table[i].entry_PT.page_size != 1) {
				assertion_failure();
				result = FAIL;
			}
		}
		else {
			if(page_directory_table[i].entry_PT.present != 0) {
				assertion_failure();
				result = FAIL;
			}
		}
	}

	// Test page table.
	for(i = 0; i < NUM_PT_SIZE; ++i) {
		if(i == VAL_184) {
			if(page_table[i].present != 1) {
				assertion_failure();
				result = FAIL;
			}
		}
		else {
			if(page_table[i].present != 0) {
				assertion_failure();
				result = FAIL;
			}
		}
	}

	return result;
}


/* Checkpoint 2 tests */
/* test_directory_operations
*
* Test functionalities in directory operation.
* Inputs: None
* Outputs: PASS/FAIL
* Coverage: Test whether directory open/close
   read/write are correct.
* Files: file_system.c
*/
int test_directory_operations(){
	TEST_HEADER;

	int result = PASS;
	int ret;
	int fd;

	ret = directory_open((uint8_t*)".");
	if(ret == -1) {
		assertion_failure();
		result = FAIL;
	}

	char buf[FILE_NAME_MAX_LENGTH + 1];
	buf[FILE_NAME_MAX_LENGTH] = '\0';
	while((ret = directory_read(fd, buf, FILE_NAME_MAX_LENGTH)) != 0) {
		if(ret == -1) {
			assertion_failure();
			result = FAIL;
		}
		printf("%s\n", buf);
	}

	// directory write must return -1
	ret = directory_write(fd, "test", 4);
	if(ret != -1) {
		assertion_failure();
		result = FAIL;
	}

	ret = directory_close(fd);
	if(ret == -1) {
		assertion_failure();
		result = FAIL;
	}

	return result;
}

/* test_file_by_name
*
* Test functionalities in file system operation.
* Inputs: None
* Outputs: PASS/FAIL
* Coverage: Test whether file open/close
   read/write are correct.
* Files: file_system.c
*/
int test_file_by_name(char *filename){
	TEST_HEADER;

	int result = PASS;
	int ret;
	int fd;
	int i;

	ret = file_open((uint8_t *)filename);
	if(ret == -1) {
		assertion_failure();
		result = FAIL;
	}

	char buf[FILE_READ_BUF_SIZE];
	ret = file_read(fd, buf, FILE_READ_BUF_SIZE);
	if(ret == -1) {
		assertion_failure();
		result = FAIL;
	}
	
	for(i = 0; i < ret; ++i)
		putc(buf[i]);
	putc('\n');

	// file write must return -1
	ret = file_write(fd, "test", sizeof("test"));
	if(ret != -1) {
		assertion_failure();
		result = FAIL;
	}

	ret = file_close(fd);
	if(ret == -1) {
		assertion_failure();
		result = FAIL;
	}

	return result;
}

/* test_file_by_index_in_boot_block
*
* Test file by reading index in boot block
* Inputs: None
* Outputs: PASS/FAIL
* Coverage: Test whether reading data through boot block
	index is correct
* Files: file_system.c
*/
int test_file_by_index_in_boot_block(int index){
	TEST_HEADER;

	int result = PASS;
	int ret;
	int i;

	dentry_t dentry;
	ret = read_dentry_by_index(index, &dentry);
	if(ret == -1) {
		assertion_failure();
		result = FAIL;
	}

	if(dentry.file_type != REGULAR_FILE) {
		assertion_failure();
		result = FAIL;
	}

	char buf[FILE_READ_BUF_SIZE];
	ret = read_data(dentry.inode_idx, 0, (uint8_t *)buf, FILE_READ_BUF_SIZE);
	if(ret == -1) {
		assertion_failure();
		result = FAIL;
	}
	
	for(i = 0; i < ret; ++i)
		putc(buf[i]);
	putc('\n');

	return result;
}

/* test_keyboard_read_and_terminal_write
*
* Test keyboard and terminal functionalities
* Inputs: None
* Outputs: PASS/FAIL
* Coverage: Test whether termial open/close/read
	/wirte are correct
* Files: keyboard.c
*/

int test_keyboard_read_and_terminal_write(){
	TEST_HEADER;

	int result = PASS;
	int ret;
	int fd;

	ret = terminal_open();
	if(ret == -1) {
		assertion_failure();
		result = FAIL;
	}

	char buf[KEYBOARD_BUFFER_CAPACITY];
	printf("Please type something:\n");
	ret = terminal_read(fd, (unsigned char*)buf, KEYBOARD_BUFFER_CAPACITY);
	if(ret == -1) {
		assertion_failure();
		result = FAIL;
	}

	printf("Printing it to screen with terminal_write():\n");
	ret = terminal_write(fd, (unsigned char*)buf, ret);
	if(ret == -1) {
		assertion_failure();
		result = FAIL;
	}

	ret = terminal_close(fd);
	if(ret == -1) {
		assertion_failure();
		result = FAIL;
	}

	return result;
}

/* test_terminal_write_size_larger_than_actual
*
* Test terminal write corner case
* Inputs: None
* Outputs: PASS/FAIL
* Coverage: Test whether termial wirte can handle
  inconsistent byte size
* Files: keyboard.c
*/

int test_terminal_write_size_larger_than_actual(){
	
	TEST_HEADER;

	int result = PASS;
	int ret;
  char string_to_display[STRING_SIZE+1] = "helloworld";
  
	ret = terminal_write(1, (unsigned char*)string_to_display, LARGER_STRING_SIZE);

	putc('\n');
	
	if(ret != STRING_SIZE) {
		assertion_failure();
		result = FAIL;
	}

	return result;
	
}

/* rtc_freq_test
*
* Test rtc frequency setting
* Inputs: None
* Outputs: PASS/FAIL
* Coverage: Test whether rtc frequency can be
	modified correctly
* Files: keyboard.c
*/

int rtc_freq_test(){

	TEST_HEADER;

	int result = PASS;
	int ret;

	int i;
	// unused fd to confirm system calls
	int32_t fd;

	char char_to_echo = '1';

	// test rtc_open()
	printf("Testing rtc_open(). Frequency should be set to 2 hz.\n");
	ret = rtc_open((uint8_t*)"");
	if(ret != 0) {
		assertion_failure();
		result = FAIL;
	}

	for(i = 0; i < RTC_TEST_COUNT; i++){
		putc(char_to_echo);
		rtc_read(fd, NULL, 0);
	}
	putc('\n');

	printf("Testing rtc_write() and rtc_read().\n");
	int32_t freq_pointer[1];
	// check rtc read/wirte under various frequencies ( under valid  range)
	for(freq_pointer[0] = VAL_2; freq_pointer[0] <= VAL_1024; freq_pointer[0] <<= 1 ){
		printf("Set frequency = %d hz\n", freq_pointer[0]);

		ret = rtc_write(fd,freq_pointer,N_BYTES_INT);
		if(ret != 0) {
			assertion_failure();
			result = FAIL;
		}

		for(i = 0; i < RTC_TEST_COUNT; i++){
			putc(char_to_echo);
			rtc_read(fd, freq_pointer, 0);
		}
		putc('\n');
	}
	
	// check invalid frequency
	freq_pointer[0] = VAL_INVALID;
	printf("Testing setting invalid frequency = %d hz.\n", freq_pointer[0]);
	ret = rtc_write(fd,freq_pointer,N_BYTES_INT);
	if(ret == 0) {
		assertion_failure();
		result = FAIL;
	}
	else {
		printf("Invalid frequency rejected by driver.\n");
	}

	printf("Testing rtc_close().\n");
	ret = rtc_close(fd);
	if(ret != 0) {
		assertion_failure();
		result = FAIL;
	}
	printf("Successfully closed.\n");

	return result;
}


/* Checkpoint 3 tests */
/* Checkpoint 4 tests */
/* Checkpoint 5 tests */


/* Test suite entry point */
void launch_tests(){
	TEST_OUTPUT("idt_test", idt_test());
	// TEST_OUTPUT("divide by 0 test", exception_de_test());
	// TEST_OUTPUT("general_exception_test", general_exception_test());
	TEST_OUTPUT("Dereference video memory address and kernel memory address", deref_valid_addresses());
	// TEST_OUTPUT("Dereference memory address that is not in page table", deref_invalid_address());
	// TEST_OUTPUT("Dereference null memory address.", deref_null_address());
	TEST_OUTPUT("PDT and PT test", pdt_and_pt_test());

	// CP2 Tests
	TEST_OUTPUT("test_directory_operations", test_directory_operations());
	TEST_OUTPUT("test_file_by_name", test_file_by_name("frame1.txt"));
	TEST_OUTPUT("test_file_by_index_in_boot_block", test_file_by_index_in_boot_block(11));
	TEST_OUTPUT("rtc_freq_test",rtc_freq_test());
	TEST_OUTPUT("test_terminal_write_size_larger_than_actual",test_terminal_write_size_larger_than_actual());
	TEST_OUTPUT("test_keyboard_read_and_terminal_write", test_keyboard_read_and_terminal_write());
}
