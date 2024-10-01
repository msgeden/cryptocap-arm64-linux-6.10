#include <linux/kernel.h>
#include <linux/printk.h>

// Global variable to store handler's stack pointer
void* ccallcret_handler_sp = NULL;

// Define the top of the handler's stack (assuming stack grows downwards)
#define HANDLER_STACK_SIZE 4096
char ccallcret_stack[HANDLER_STACK_SIZE];
char* ccallcret_stack_top;

// Declaration of external assembly functions
//__attribute__((section(".ccall_handler_code")))
extern void ccall_handler(uint64_t target_ttbr, uint64_t target_pc);
//__attribute__((section(".cret_handler_code")))
extern void cret_handler(void);


// Initialization function
static int __init init_ccallcret_handler_stack(void) {
    ccallcret_stack_top = ccallcret_stack + HANDLER_STACK_SIZE;
    ccallcret_handler_sp = ccallcret_stack_top;
    printk(KERN_INFO "CCALLCRET Handler stack initialized\n");
    return 0;
}

// Cleanup function (if needed)
static void __exit cleanup_ccallcret_handler_stack(void) {
    // Perform any necessary cleanup here
    printk(KERN_INFO "CCALLCRET Handler stack cleaned up\n");
}

// Register the handler to run during late initialization
late_initcall(init_ccallcret_handler_stack);
//late_exitcall(cleanup_ccallcret_handler_stacks);


// Function to print all general-purpose registers (GPRs) on ARM64 without modifying them
void ccallcret_print_gpr_values(void) {
    uint64_t x0, x1, x2, x3, x4, x5, x6, x7;
    uint64_t x8, x9, x10, x11, x12, x13, x14, x15;
    uint64_t x16, x17, x18, x19, x20, x21, x22, x23;
    uint64_t x24, x25, x26, x27, x28, x29, x30;
    uint64_t ttbr0_el1, spsr_el1, elr_el1;
    // Save all GPRs onto the stack
    asm volatile (
        "sub sp, sp, #272\n"        // Make space on the stack for all GPRs (256 bytes for 32 registers)
        "stp x0, x1, [sp, #0]\n"    // Store x0 and x1
        "stp x2, x3, [sp, #16]\n"   // Store x2 and x3
        "stp x4, x5, [sp, #32]\n"   // Store x4 and x5
        "stp x6, x7, [sp, #48]\n"   // Store x6 and x7
        "stp x8, x9, [sp, #64]\n"   // Store x8 and x9
        "stp x10, x11, [sp, #80]\n" // Store x10 and x11
        "stp x12, x13, [sp, #96]\n" // Store x12 and x13
        "stp x14, x15, [sp, #112]\n"// Store x14 and x15
        "stp x16, x17, [sp, #128]\n"// Store x16 and x17
        "stp x18, x19, [sp, #144]\n"// Store x18 and x19
        "stp x20, x21, [sp, #160]\n"// Store x20 and x21
        "stp x22, x23, [sp, #176]\n"// Store x22 and x23
        "stp x24, x25, [sp, #192]\n"// Store x24 and x25
        "stp x26, x27, [sp, #208]\n"// Store x26 and x27
        "stp x28, x29, [sp, #224]\n"// Store x28 (FP) and x29 (LR)
        "str x30, [sp, #240]\n"     // Store x30 (LR)
   
    );

    // Load the register values from the stack into C variables
    asm volatile (
        "ldp %0, %1, [sp, #0]\n"
        "ldp %2, %3, [sp, #16]\n"
        : "=r"(x0), "=r"(x1), "=r"(x2), "=r"(x3)
    );
    
    asm volatile (
        "ldp %0, %1, [sp, #32]\n"
        "ldp %2, %3, [sp, #48]\n"
        : "=r"(x4), "=r"(x5), "=r"(x6), "=r"(x7)
    );

    asm volatile (
        "ldp %0, %1, [sp, #64]\n"
        "ldp %2, %3, [sp, #80]\n"
        : "=r"(x8), "=r"(x9), "=r"(x10), "=r"(x11)
    );

    asm volatile (
        "ldp %0, %1, [sp, #96]\n"
        "ldp %2, %3, [sp, #112]\n"
        : "=r"(x12), "=r"(x13), "=r"(x14), "=r"(x15)
    );

    asm volatile (
        "ldp %0, %1, [sp, #128]\n"
        "ldp %2, %3, [sp, #144]\n"
        : "=r"(x16), "=r"(x17), "=r"(x18), "=r"(x19)
    );

    asm volatile (
        "ldp %0, %1, [sp, #160]\n"
        "ldp %2, %3, [sp, #176]\n"
        : "=r"(x20), "=r"(x21), "=r"(x22), "=r"(x23)
    );

    asm volatile (
        "ldp %0, %1, [sp, #192]\n"
        "ldp %2, %3, [sp, #208]\n"
        : "=r"(x24), "=r"(x25), "=r"(x26), "=r"(x27)
    );

    asm volatile (
        "ldp %0, %1, [sp, #224]\n"
        "ldr %2, [sp, #240]\n"
        : "=r"(x28), "=r"(x29), "=r"(x30)
    );

    // Read ttbr0_el1, spsr_el1, and elr_el1 registers
    asm volatile (
        "mrs %0, ttbr0_el1\n"   // Read ttbr0_el1
        "mrs %1, spsr_el1\n"    // Read spsr_el1
        "mrs %2, elr_el1\n"     // Read elr_el1
        : "=r"(ttbr0_el1), "=r"(spsr_el1), "=r"(elr_el1)
    );
    
    // Print the values of all GPRs
    printk(KERN_INFO "Register x0:  0x%llx\n", x0);
    printk(KERN_INFO "Register x1:  0x%llx\n", x1);
    printk(KERN_INFO "Register x2:  0x%llx\n", x2);
    printk(KERN_INFO "Register x3:  0x%llx\n", x3);
    printk(KERN_INFO "Register x4:  0x%llx\n", x4);
    printk(KERN_INFO "Register x5:  0x%llx\n", x5);
    printk(KERN_INFO "Register x6:  0x%llx\n", x6);
    printk(KERN_INFO "Register x7:  0x%llx\n", x7);
    printk(KERN_INFO "Register x8:  0x%llx\n", x8);
    printk(KERN_INFO "Register x9:  0x%llx\n", x9);
    printk(KERN_INFO "Register x10: 0x%llx\n", x10);
    printk(KERN_INFO "Register x11: 0x%llx\n", x11);
    printk(KERN_INFO "Register x12: 0x%llx\n", x12);
    printk(KERN_INFO "Register x13: 0x%llx\n", x13);
    printk(KERN_INFO "Register x14: 0x%llx\n", x14);
    printk(KERN_INFO "Register x15: 0x%llx\n", x15);
    printk(KERN_INFO "Register x16: 0x%llx\n", x16);
    printk(KERN_INFO "Register x17: 0x%llx\n", x17);
    printk(KERN_INFO "Register x18: 0x%llx\n", x18);
    printk(KERN_INFO "Register x19: 0x%llx\n", x19);
    printk(KERN_INFO "Register x20: 0x%llx\n", x20);
    printk(KERN_INFO "Register x21: 0x%llx\n", x21);
    printk(KERN_INFO "Register x22: 0x%llx\n", x22);
    printk(KERN_INFO "Register x23: 0x%llx\n", x23);
    printk(KERN_INFO "Register x24: 0x%llx\n", x24);
    printk(KERN_INFO "Register x25: 0x%llx\n", x25);
    printk(KERN_INFO "Register x26: 0x%llx\n", x26);
    printk(KERN_INFO "Register x27: 0x%llx\n", x27);
    printk(KERN_INFO "Register x28: 0x%llx\n", x28);
    printk(KERN_INFO "Register x29 (FP): 0x%llx\n", x29);  // x29 is the Frame Pointer (FP)
    //printk(KERN_INFO "Register x30 (LR): 0x%llx\n", x30);  // x30 is the Link Register (LR)

    // Print the values of the system registers
    printk(KERN_INFO "TTBR0_EL1:  0x%llx\n", ttbr0_el1);
    printk(KERN_INFO "SPSR_EL1:   0x%llx\n", spsr_el1);
    printk(KERN_INFO "ELR_EL1:    0x%llx\n", elr_el1);

    // Restore all GPRs from the stack
    asm volatile (
        "ldp x0, x1, [sp, #0]\n"
        "ldp x2, x3, [sp, #16]\n"
        "ldp x4, x5, [sp, #32]\n"
        "ldp x6, x7, [sp, #48]\n"
        "ldp x8, x9, [sp, #64]\n"
        "ldp x10, x11, [sp, #80]\n"
        "ldp x12, x13, [sp, #96]\n"
        "ldp x14, x15, [sp, #112]\n"
        "ldp x16, x17, [sp, #128]\n"
        "ldp x18, x19, [sp, #144]\n"
        "ldp x20, x21, [sp, #160]\n"
        "ldp x22, x23, [sp, #176]\n"
        "ldp x24, x25, [sp, #192]\n"
        "ldp x26, x27, [sp, #208]\n"
        "ldp x28, x29, [sp, #224]\n"
        "ldr x30, [sp, #240]\n"
   
        "add sp, sp, #272\n"  // Restore the stack pointer back to its original position
    );
}