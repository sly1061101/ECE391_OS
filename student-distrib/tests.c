#include "tests.h"
#include "x86_desc.h"
#include "lib.h"

#define PASS 1
#define FAIL 0

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
void exception_de_test(){
    TEST_HEADER;
    int a = 5;
    int b = 0;
    int c;
	c = a/b;
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
void general_exception_test(){
    TEST_HEADER;

	// Replace the interrupt number to what you want to trigger.
	asm volatile("int $10");
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
	p = 0x000B8000;
	printf("The byte stored at address 0x%#x is 0x%x.\n", p, *p);

	// Kernel memory page.
	p = 0x00400000;
	printf("The byte stored at address 0x%#x is 0x%x.\n", p, *p);
	p = 0x00500000;
	printf("The byte stored at address 0x%#x is 0x%x.\n", p, *p);
	p = 0x00600000;
	printf("The byte stored at address 0x%#x is 0x%x.\n", p, *p);
	p = 0x00700000;
	printf("The byte stored at address 0x%#x is 0x%x.\n", p, *p);
	p = 0x007FFFFF;
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
void deref_invalid_address(){
    TEST_HEADER;

	uint8_t *p;

	// Set this to be an invalid address.
	p = 0x000A0000;
	printf("The byte stored at address 0x%#x is 0x%x.\n", p, *p);
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
void deref_null_address(){
    TEST_HEADER;

	uint8_t *p = NULL;
	printf("The byte stored at address 0x%#x is 0x%x.\n", p, *p);
}

/* Checkpoint 2 tests */
/* Checkpoint 3 tests */
/* Checkpoint 4 tests */
/* Checkpoint 5 tests */


/* Test suite entry point */
void launch_tests(){
	TEST_OUTPUT("idt_test", idt_test());
	// TEST_OUTPUT("divide by 0 test", exception_de_test());
	// TEST_OUTPUT("general_exception_test", general_exception_test());
	// TEST_OUTPUT("Dereference video memory address and kernel memory address", deref_valid_addresses());
	// TEST_OUTPUT("Dereference memory address that is not in page table", deref_invalid_address());
	// TEST_OUTPUT("Dereference null memory address.", deref_null_address());
}
