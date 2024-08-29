// #include <linux/init.h>
// #include <linux/kernel.h>
// #include <linux/printk.h>

// __attribute__((section(".ccall_handler")))
// static char handler_stack[4096];  // 4KB stack

// __attribute__((section(".ccall_handler")))
// void ccall_handler(void)
// {
// 	printk(KERN_INFO "Hello from the ccall handler at a predefined address!\n");
// 	// Handler code
// }

// // // __attribute__((section(".cret_handler")))
// // // void cret_handler(void)
// // // {
// // // 	printk(KERN_INFO "Hello from the cret handler at a predefined address!\n");
// // // 	// Handler code
// // // }
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/printk.h>

#define PAGE_SIZE 4096
#define STACK_SIZE (5*PAGE_SIZE)  // Example stack size (4KB)

__attribute__((section(".ccall_handler_data")))
static char handler_stack[STACK_SIZE] __aligned(PAGE_SIZE);  // Aligned stack


// int __init ccall_handler(void)
// {
//     printk(KERN_INFO "Running ccall_handler during boot\n");
//     // Your handler code here

//     return 0;  // Return 0 to indicate success
// }



__attribute__((section(".ccall_handler_code")))
int ccall_handler(void)
{
    // Switch to handler stack
		// asm volatile (
		//     "mov x1, %0\n"
		//     "mov sp, x1\n"
		//     :
		//     : "r" (handler_stack + STACK_SIZE)
		//     : "x1"
		// );

    // // Save CPU context (example saving x27, x28, x29, and x30)
    // asm volatile (
    //     "stp x29, x30, [sp, #-16]!\n"
    //     "stp x27, x28, [sp, #-16]!\n"
    // );

    // Handler code
    printk(KERN_INFO "Handler running with dedicated stack\n");

    // // Restore CPU context
    // asm volatile (
    //     "ldp x27, x28, [sp], #16\n"
    //     "ldp x29, x30, [sp], #16\n"
    // );
    return 0;
    // Restore original stack pointer before returning
}

// Register the handler to run during late initialization
late_initcall(ccall_handler);
