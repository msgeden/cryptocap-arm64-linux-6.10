// Declare external variables declared ccallcret_handler.c 
.extern ccallcret_handler_sp         
.extern ccallcret_handler_stack_top       
.extern ccallcret_print_gpr_values   

// ccall_handler.S
.section .ccall_handler_code, "ax"
.global ccall_handler
ccall_handler:

    //DISABLE INTERRUPTS
    //mrs x0, daif
    //msr daifset, #0xf

    //Use x10 as our the SP of our custom stack (handler_sp)
    ldr x9, =ccallcret_handler_sp        
    ldr x10, [x9]        

    //CHECKING REASON OF THE BUG with SP_EL1 and TTBR1_EL1
    //mrs	x9, sp_el1
    //mov x9, sp
    //str	x9, [x10, #-8]!
    //mrs	x9, ttbr1_el1
    //str	x9, [x10, #-8]!

    //Push ttbr0_el1, elr_el1, spsr_el1
    mrs	x9, ttbr0_el1
    str	x9, [x10, #-8]!
    mrs	x9, elr_el1
    str	x9, [x10, #-8]!
    mrs	x9, spsr_el1
    str	x9, [x10, #-8]!

    mrs	x9, sp_el0
    str	x9, [x10, #-8]!
    
    //Push x30-x2 excluding x9, x10, x0, x1
    str	x30, [x10, #-8]!
    str	x29, [x10, #-8]!
    str	x28, [x10, #-8]!
    str	x27, [x10, #-8]!
    str	x26, [x10, #-8]!
    str	x25, [x10, #-8]!
    str	x24, [x10, #-8]!
    str	x23, [x10, #-8]!
    str	x22, [x10, #-8]!
    str	x21, [x10, #-8]!
    str	x20, [x10, #-8]!
    str	x19, [x10, #-8]!
    str	x18, [x10, #-8]!
    str	x17, [x10, #-8]!
    str	x16, [x10, #-8]!
    str	x15, [x10, #-8]!
    str	x14, [x10, #-8]!
    str	x13, [x10, #-8]!
    str	x12, [x10, #-8]!
    str	x11, [x10, #-8]!
    
    str	x8, [x10, #-8]!
    str	x7, [x10, #-8]!
    str	x6, [x10, #-8]!
    str	x5, [x10, #-8]!
    str	x4, [x10, #-8]!
    str	x3, [x10, #-8]!
    //str	x2, [x10, #-8]!
    //str	x1, [x10, #-8]!
    //str	x0, [x10, #-8]!

    // Update the top of the stack
    ldr x9, =ccallcret_handler_sp        
    str	x10, [x9]
    
    // Reset condition flags in spsr1_el1
    //mrs	x9, spsr_el1
    //and	x9, x9, #0xfffffff
    //msr	spsr_el1, x9

    //TODO: Sign cap arguments here

    // Set user SP using CLC.SP as the callee's SP
    .word 0x03700049 //clsp #0, x9
    msr	sp_el0, x9

    // Set elr_el1 using CLC.PC for the address to be jumped
    .word 0x03600049 //clpc #0, x9
    msr	elr_el1, x9
  
    // Set ttbr0_el1 using CLC.PT for the address space to be jumped
    .word 0x03800049 //clpt #0, x9
    msr	ttbr0_el1, x9


    //CHECKING REASON OF THE BUG with SP_EL1 and TTBR1_EL1
    // Set Kernel SP using CLC.MAC for the address space to be jumped
    //.word 0x03b00049 //clmac #0, x9
    //msr	ttbr1_el1, x1
    
    //mov sp, x9
    //msr	sp_el1, x9

    //(Instruction Synchronization Barrier)
    isb
    
    //(Translation Lookaside Buffer Invalidate - EL1) 
    tlbi vmalle1
    
    //TLB Invalidate by Virtual Address - EL1, Inner Shareable)
    //tlbi vae1is
    
    //(Data Synchronization Barrier - Full System)
    dsb sy
    // Exit from EL1

    //EMABLE INTERRUPTS
    //msr daif, x0

    .word	0x03500000 //cjmp (i.e. eret)

    
.section .cret_handler_code, "ax"
.global cret_handler  
cret_handler:
 
    //DISABLE INTERRUPTS
    //mrs x0, daif
    //msr daifset, #0xf
    
    //Use x10 as our the SP of our custom stack (handler_sp)
    ldr x9, =ccallcret_handler_sp        
    ldr x10, [x9]         

    //ldr x0, [x10], #8
    //ldr x1, [x10], #8
    //ldr x2, [x10], #8
    ldr x3, [x10], #8
    ldr x4, [x10], #8
    ldr x5, [x10], #8
    ldr x6, [x10], #8
    ldr x7, [x10], #8
    ldr x8, [x10], #8

    ldr x11, [x10], #8
    ldr x12, [x10], #8
    ldr x13, [x10], #8
    ldr x14, [x10], #8
    ldr x15, [x10], #8
    ldr x16, [x10], #8
    ldr x17, [x10], #8
    ldr x18, [x10], #8
    ldr x19, [x10], #8
    ldr x20, [x10], #8
    ldr x21, [x10], #8
    ldr x22, [x10], #8
    ldr x23, [x10], #8
    ldr x24, [x10], #8
    ldr x25, [x10], #8
    ldr x26, [x10], #8
    ldr x27, [x10], #8
    ldr x28, [x10], #8
    ldr x29, [x10], #8
    ldr x30, [x10], #8

    // Restore SP
    ldr x9, [x10], #8
    msr sp_el0, x9

    // Restore system registers (`SPSR_EL1` `ELR_EL1`, `TTBR0_EL1`) from our custom stack
    ldr x9, [x10], #8
    msr spsr_el1, x9
    ldr x9, [x10], #8
    msr elr_el1, x9

    //this is to locate where the SP should be (since ttbr0_el1 cannot be popped before setting ccallcret_handler_sp)
   
    add x10, x10, #8
    //CHECKING REASON OF THE BUG with SP_EL1 and TTBR1_EL1
    //add x10, x10, #24
   
    

    // Update the top of the stack
    ldr x9, =ccallcret_handler_sp        
    str	x10, [x9]

    //this is necessary to retrieve ttbr0_el1 value
    ldr x9, [x10, #-8]
    msr ttbr0_el1, x9

    //CHECKING REASON OF THE BUG with SP_EL1 and TTBR1_EL1
    //ldr x9, [x10, #-8]
    //msr ttbr1_el1, x9

    //ldr x9, [x10, #-8]
    //msr sp_el1, x9


    //(Instruction Synchronization Barrier)
    isb
    
    //(Translation Lookaside Buffer Invalidate - EL1) 
    tlbi vmalle1
    
    //TLB Invalidate by Virtual Address - EL1, Inner Shareable)
    //tlbi vae1is
    
    //(Data Synchronization Barrier - Full System)
    dsb sy
    // Exit from EL1

    //EMABLE INTERRUPTS
    //msr daif, x0

    //cjmp Exit from EL1
    .word 0x3500000 //cjmp (i.e. eret)