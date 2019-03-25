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
#define COUNT 30
#define STRING_SIZE 10
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
	for (i = 0; i < 10; ++i){
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
    int a = 5;
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
		if(i == 184) {
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

int test_directory_operations(){
	TEST_HEADER;

	int result = PASS;
	int ret;
	int fd;

	ret = directory_open(".");
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

int test_file_by_name(char *filename){
	TEST_HEADER;

	int result = PASS;
	int ret;
	int i;

	ret = file_open(filename);
	if(ret == -1) {
		assertion_failure();
		result = FAIL;
	}

	char buf[10000];
	ret = file_read(0, buf, 10000);
	if(ret == -1) {
		assertion_failure();
		result = FAIL;
	}
	
	for(i = 0; i < ret; ++i)
		putc(buf[i]);
	putc('\n');

	return result;
}

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

	char buf[10000];
	ret = read_data(dentry.inode_idx, 0, buf, 10000);
	if(ret == -1) {
		assertion_failure();
		result = FAIL;
	}
	
	for(i = 0; i < ret; ++i)
		putc(buf[i]);
	putc('\n');

	return result;
}

int test_keyboard_read_and_terminal_write(){
	TEST_HEADER;

	int result = PASS;
	int ret;

	char buf[128];
	printf("Please type something:\n");
	ret = terminal_read(1, buf, 128);
	if(ret == -1) {
		assertion_failure();
		result = FAIL;
	}

	printf("Printing it to screen with terminal_write():\n");
	ret = terminal_write(1, buf, ret);
	if(ret == -1) {
		assertion_failure();
		result = FAIL;
	}

	return result;
}

int test_terminal_write_invalid(){
	
	TEST_HEADER;

	int result = PASS;
	int ret;
  char string_to_display[STRING_SIZE+1] = "helloworld";
  
	ret = terminal_write(1, string_to_display, 20);

	putc('\n');
	
	if(ret != STRING_SIZE) {
		assertion_failure();
		result = FAIL;
	}

	return result;
	
}

int rtc_freq_test(){

	TEST_HEADER;

	int result = PASS;
	int ret;

	int i;
	// unused fd to confirm system calls
	int32_t fd;

	// unused fd
	int32_t freq_pointer[1];

	// check rtc read/wirte under various frequencies ( under valid  range)
	for(freq_pointer[0] = VAL_2; freq_pointer[0] <= VAL_1024; freq_pointer[0] <<= 1 ){
		printf("Current frequency = %d hz\n", freq_pointer[0]);

		ret = rtc_write(fd,freq_pointer,N_BYTES_INT);
		if(ret != 0) {
			assertion_failure();
			result = FAIL;
		}

		for(i = 0; i < COUNT; i++){
			putc('1');
			rtc_read(fd, freq_pointer, 0);
		}
		putc('\n');
	}
	
	// check invalid frequency
	freq_pointer[0] = VAL_INVALID;
	printf("Testing invalid frequency = %d hz.\n", freq_pointer[0]);
	ret = rtc_write(fd,freq_pointer,N_BYTES_INT);
	if(ret == 0) {
		assertion_failure();
		result = FAIL;
	}
	else {
		printf("Pass. Invalid frequency rejected by driver.\n");
	}

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
	TEST_OUTPUT("test_terminal_write_invalid",test_terminal_write_invalid());
	TEST_OUTPUT("test_keyboard_read_and_terminal_write", test_keyboard_read_and_terminal_write());
}
