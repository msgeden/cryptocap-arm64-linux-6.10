// Declare external variables declared ccallcret_handler.c 
.extern ccallcret_handler_sp         
.extern ccallcret_handler_stack_top       
.extern ccallcret_print_gpr_values   

// ccall_handler.S
.section .ccall_handler_code, "ax"
.global ccall_handler
ccall_handler:
    bl	ccallcret_print_gpr_values
    
    //Use x10 as our the SP of our custom stack (handler_sp)
    ldr x9, =ccallcret_handler_sp        
    ldr x10, [x9]        

    //Push ttbr0_el1, elr_el1, spsr_el1
    mrs	x9, ttbr0_el1
    str	x9, [x10, #-8]!
    mrs	x9, elr_el1
    str	x9, [x10, #-8]!
    mrs	x9, spsr_el1
    str	x9, [x10, #-8]!
 
    //Push x30-x2 excluding x9, x10, x0, x1
    str	x30, [x10, #-8]!
    stp	x28, x29, [x10, #-16]!
    stp	x26, x27, [x10, #-16]!
    stp	x24, x25, [x10, #-16]!
    stp	x22, x23, [x10, #-16]!
    stp	x20, x21, [x10, #-16]!
    stp	x18, x19, [x10, #-16]!
    stp	x16, x17, [x10, #-16]!
    stp	x14, x15, [x10, #-16]!
    stp	x12, x13, [x10, #-16]!
    str	x11, [x10, #-8]!
    str	x8, [x10, #-8]!
    stp	x6, x7, [x10, #-16]!
    stp	x4, x5, [x10, #-16]!
    stp	x2, x3, [x10, #-16]!

    // Update the top of the stack
    ldr x9, =ccallcret_handler_sp        
    str	x10, [x9]
    
    // Reset condition flags in spsr1_el1
    mrs	x9, spsr_el1
    and	x9, x9, #0xfffffff
    msr	spsr_el1, x9

    // Set elr_el1 using second argument (x1) for the address to be jumped
    msr	elr_el1, x1
    // Set ttbr0_el1 using first argument (x0) for the address space to be jumped
    msr	ttbr0_el1, x0
    isb
    // Exit from EL1
    .word	0x03500000

.section .cret_handler_code, "ax"
.global cret_handler  
cret_handler:
    //Use x10 as our the SP of our custom stack (handler_sp)
    ldr x9, =ccallcret_handler_sp        
    ldr x10, [x9]         
    
    ldp x2, x3, [x10], #16
    ldp x4, x5, [x10], #16
    ldp x6, x7, [x10], #16
    ldr x8, [x10], #8
    ldr x11, [x10], #8
    ldp x12, x13, [x10], #16
    ldp x14, x15, [x10], #16
    ldp x16, x17, [x10], #16
    ldp x18, x19, [x10], #16
    ldp x20, x21, [x10], #16
    ldp x22, x23, [x10], #16
    ldp x24, x25, [x10], #16
    ldp x26, x27, [x10], #16
    ldp x28, x29, [x10], #16
    ldr x30, [x10], #8

    // Restore system registers (`SPSR_EL1` `ELR_EL1`, `TTBR0_EL1`) from our custom stack
    ldr x9, [x10], #8
    msr spsr_el1, x9
    ldr x9, [x10], #8
    msr elr_el1, x9
    ldr x9, [x10], #8
    
    // Update the top of the stack
    ldr x9, =ccallcret_handler_sp        
    str	x10, [x9]

    bl ccallcret_print_gpr_values 
    
    msr ttbr0_el1, x9
    isb
    //cjmp Exit from EL1
    .word 0x3500000
