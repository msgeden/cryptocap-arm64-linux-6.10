// SPDX-License-Identifier: GPL-2.0-only
/*
 * AArch64-specific system calls implementation
 *
 * Copyright (C) 2012 ARM Ltd.
 * Author: Catalin Marinas <catalin.marinas@arm.com>
 */

#include <linux/compiler.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/export.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/syscalls.h>

#include <asm/cpufeature.h>
#include <asm/syscall.h>

SYSCALL_DEFINE6(mmap, unsigned long, addr, unsigned long, len,
		unsigned long, prot, unsigned long, flags,
		unsigned long, fd, unsigned long, off)
{
	if (offset_in_page(off) != 0)
		return -EINVAL;

	return ksys_mmap_pgoff(addr, len, prot, flags, fd, off >> PAGE_SHIFT);
}

SYSCALL_DEFINE1(arm64_personality, unsigned int, personality)
{
	if (personality(personality) == PER_LINUX32 &&
		!system_supports_32bit_el0())
		return -EINVAL;
	return ksys_personality(personality);
}


//#ifdef TARGET_CRYPTO_CAP
//#463
SYSCALL_DEFINE1(cdummy,  uint64_t, ret_val)
{
	int ret=0;
	printk(KERN_INFO "cdummy is called with ret_val:%ld",ret_val);
	return ret_val;
}
//#464
//#ifdef TARGET_CRYPTO_CAP
// SYSCALL_DEFINE0(ccall)
// {
// 	volatile uint64_t ttbr0, elr, spsr, sp_el0_current; 

// 	asm volatile ("mrs %0, ttbr0_el1" : "=r"(current->saved_ttbr0_el1));
//     asm volatile ("mrs %0, elr_el1" : "=r"(current->saved_elr_el1));
//     asm volatile ("mrs %0, spsr_el1" : "=r"(current->saved_spsr_el1));
//     asm volatile ("mrs %0, sp_el0" : "=r"(current->saved_sp_el0));

// 	ttbr0 = current->saved_ttbr0_el1;
//     elr = current->saved_elr_el1;
//     spsr = current->saved_spsr_el1;
//     sp_el0_current=current->saved_sp_el0;

// 	// Set user SP using CLC.SP as the callee's SP
//     //asm volatile (".word 0x03700049"); //clsp #0, x9
//     //asm volatile ("msr	sp_el0, x9");
 
//     // Reset condition flags in spsr1_el1
//     //mrs	x9, spsr_el1
//     //and	x9, x9, #0xfffffff
//     //msr	spsr_el1, x9

//     // Set elr_el1 using CLC.PC for the address to be jumped
//     asm volatile (".word 0x03600049"); //clpc #0, x9

//     asm volatile ("msr	elr_el1, x9");
  
//     // Set ttbr0_el1 using CLC.PT for the address space to be jumped
//     asm volatile (".word 0x03800049"); //clpt #0, x9
//     asm volatile ("msr	ttbr0_el1, x9");

//     //(Instruction Synchronization Barrier)
// 	asm volatile ("isb");
//     //https://developer.arm.com/documentation/ddi0488/c/system-control/aarch64-register-summary/aarch64-tlb-maintenance-operations
//     asm volatile ("tlbi vmalle1");
//     //(Data Synchronization Barrier - Full System)
//     asm volatile ("dsb sy");
	
// 	asm volatile ("eret");

// 	return 0;
// }
// SYSCALL_DEFINE0(cret)
// {
// 	volatile uint64_t ttbr0, elr, spsr, sp_el0_current;
	
//     // Retrieve system registers from task_struct
//     sp_el0_current = current->saved_sp_el0;
//     spsr = current->saved_spsr_el1;
//     elr = current->saved_elr_el1;
// 	ttbr0 = current->saved_ttbr0_el1;
    
// 	// Set the system registers with the retrieved values
//     // asm volatile ("msr spsr_el1, %0" : : "r"(spsr));   // Set SPSR
//     asm volatile ("msr elr_el1, %0" : : "r"(elr));     // Set ELR
//     asm volatile ("msr ttbr0_el1, %0" : : "r"(ttbr0)); // Set TTBR0
    
//     //(Instruction Synchronization Barrier)
// 	asm volatile ("isb");
//     //https://developer.arm.com/documentation/ddi0488/c/system-control/aarch64-register-summary/aarch64-tlb-maintenance-operations
//     asm volatile ("tlbi vmalle1");
//     //(Data Synchronization Barrier - Full System)
//     asm volatile ("dsb sy");
	
// 	asm volatile ("eret");

// 	return 0;
// }
SYSCALL_DEFINE2(ccall, pid_t, target_pid, uint64_t, target_pc) {
    
    struct task_struct *callee;
    struct pt_regs *callee_regs;

    printk(KERN_INFO "ccall entry: target_pid:%ld, target_pc:0x%lx\n", target_pid, target_pc);

    callee = find_task_by_vpid(target_pid);
    if (!callee)
        return -ESRCH;

    // Save caller's context
    memcpy(&callee->ccaller_info.caller_regs, task_pt_regs(current), sizeof(struct pt_regs));
    callee->ccaller_info.caller_task = current;
  
    // Set the target process's PC to the function address
    callee_regs = task_pt_regs(callee);
    callee_regs->pc = target_pc;
   
   
    // Direct switch to target process
    callee->__state = TASK_RUNNING;

    printk(KERN_INFO "ccall prior to cpu_switch");

    // Switch to target process
    cpu_switch_to(current, callee);

    printk(KERN_INFO "ccall return");

    return 0;
}

SYSCALL_DEFINE0(cret)
{
    struct task_struct *caller;
    struct pt_regs *caller_regs;

    printk(KERN_INFO "cret entry");

    caller = current->ccaller_info.caller_task;
    caller_regs = &current->ccaller_info.caller_regs;

    memcpy(task_pt_regs(current), caller_regs, sizeof(struct pt_regs));
    
    caller->__state = TASK_RUNNING;

    printk(KERN_INFO "cret prior to cpu_switch");

    cpu_switch_to(current, caller);

    printk(KERN_INFO "cret return");

	return 0;
}
#define MAX_NESTED_CALLS 16

struct caller_data {
    struct task_struct *caller_task;
    pid_t caller_pid;
    uint64_t ret_val;
};

// Stack of caller data
static struct caller_data caller_stack[MAX_NESTED_CALLS];
// Stack of completion variables
static struct completion pcall_done_stack[MAX_NESTED_CALLS];

// Top of the stack (index of the next free slot)
static int call_stack_top = 0;

// A lock to protect stack operations
static DEFINE_SPINLOCK(stack_lock);

SYSCALL_DEFINE3(pcall, pid_t, target_pid, uint64_t, target_pc, uint64_t, target_mac) {

    struct task_struct *target_task;
    struct pt_regs *regs;
    int idx;

    printk(KERN_INFO "pcall entry: target_pid:%ld, target_pc:0x%lx\n", target_pid, target_pc);

    // There is no need for encryption/decryption of TCR (TID) value as it can be accessed only via EL1 with new design  
    // Update/Roll TCR value
    asm volatile(
            ".word 0x2a00009\n\t"     // readtcr x9
            "add x9, x9, #1\n\t"      // increment x9
            ".word 0x2b00009\n\t"     // updtcr x9
            :
            :
            : "x9"
    );

    // (Re)sign capability registers (CRx)
    asm volatile(
            ".word 0x02900000\n\t"     // csign cr0
            ".word 0x02900001\n\t"     // csign cr1
            ".word 0x02900002\n\t"     // csign cr2
            ".word 0x02900003\n\t"     // csign cr3
            ".word 0x02900004\n\t"     // csign cr4
            ".word 0x02900005\n\t"     // csign cr5
            ".word 0x02900006\n\t"     // csign cr6
            ".word 0x02900007\n\t"     // csign cr7
    );

    target_task = find_task_by_vpid(target_pid);
    if (!target_task) {
        printk(KERN_ERR "pcall error: Target task not found.\n");
        return -ESRCH;
    }

    spin_lock(&stack_lock);
    if (call_stack_top >= MAX_NESTED_CALLS) {
        spin_unlock(&stack_lock);
        return -ENOMEM;  // No space left for a new nested call
    }

    idx = call_stack_top++;

    // Initialize a new completion for this call
    init_completion(&pcall_done_stack[idx]);

    // Save the caller
    caller_stack[idx].caller_task = current;
    caller_stack[idx].caller_pid = task_pid_nr(current);
    spin_unlock(&stack_lock);

    // Set the target process's PC
    regs = task_pt_regs(target_task);
    regs->pc = target_pc;

    wake_up_process(target_task);

    // Wait for the corresponding `pret`
    wait_for_completion(&pcall_done_stack[idx]);

    spin_lock(&stack_lock);
    // Retrieve return value after pret completes
    uint64_t ret_val = caller_stack[idx].ret_val;
    // Pop the stack
    call_stack_top--;
    spin_unlock(&stack_lock);

    printk(KERN_INFO "pcall termination\n");

    return ret_val;
}

SYSCALL_DEFINE1(pret, uint64_t, ret_val) {
    int idx;

    printk(KERN_INFO "pret entry: current pid:%d, ret_val:%ld\n", task_pid_nr(current), ret_val);
  
    // Update/Unroll TCR value
    asm volatile(
            ".word 0x2a00009\n\t"     // readtcr x9
            "sub x9, x9, #1\n\t"      // decrement x9
            ".word 0x2b00009\n\t"     // updtcr x9
            :
            :
            : "x9"
    );

    spin_lock(&stack_lock);
    if (call_stack_top == 0) {
        spin_unlock(&stack_lock);
        return -EPERM;  // No active pcall to return from
    }

    // The pret corresponds to the most recent pcall (last on the stack)
    idx = call_stack_top - 1;

    // Check if the caller pid matches and that current isn't the caller
    if (task_pid_nr(current) == caller_stack[idx].caller_pid) {
        spin_unlock(&stack_lock);
        return -EPERM;  // The original caller can't call pret
    }

    // Set return value
    caller_stack[idx].ret_val = ret_val;
    {
        struct pt_regs *regs = task_pt_regs(caller_stack[idx].caller_task);
        regs->regs[0] = ret_val;
    }

    // Wake up the original caller
    wake_up_process(caller_stack[idx].caller_task);

    // Signal the pcall completion
    complete(&pcall_done_stack[idx]);
    spin_unlock(&stack_lock);

    printk(KERN_INFO "pret termination\n");
    return ret_val;
}

SYSCALL_DEFINE2(dcall, pid_t, target_pid, uint64_t, target_pc) {
   
    struct task_struct *callee;
    struct pt_regs *callee_regs;
    preempt_disable();  // Disable preemption before context switch

    
    volatile uint64_t current_elr_el1;
    asm volatile("mrs %0, elr_el1" : "=r" (current_elr_el1));
    current->caller_ret_pc=current_elr_el1;
    printk(KERN_INFO "dcall entry: target_pid:%ld, target_pc:0x%lx, return_pc:0x%lx\n", target_pid, target_pc, current_elr_el1);
    
    callee = find_task_by_vpid(target_pid);
    if (!callee)
        return -ESRCH;

    callee_regs = task_pt_regs(callee);
    callee_regs->pc = target_pc;

    
    cpu_switch_to(current, callee);


    printk(KERN_INFO "dcall termination\n");
    return 0;
}  
SYSCALL_DEFINE1(dret, pid_t, return_pid) {

    struct task_struct *caller;
    struct pt_regs *caller_regs;
    preempt_disable();  // Disable preemption before context switch

    printk(KERN_INFO "dret entry: return_pid:%ld\n", return_pid);

    caller = find_task_by_vpid(return_pid);
    if (!caller)
        return -ESRCH;

    caller_regs = task_pt_regs(caller);
    caller_regs->pc = caller->caller_ret_pc;
    
    cpu_switch_to(current, caller);

    printk(KERN_INFO "dret termination\n");

    return 0;
}  
SYSCALL_DEFINE2(acall, pid_t, target_pid, uint64_t, target_pc) {
    printk(KERN_INFO "acall entry\n");
    printk(KERN_INFO "acall termination\n");
    return 0;
}  
SYSCALL_DEFINE0(aret) {
    printk(KERN_INFO "aret entry\n");
    printk(KERN_INFO "aret termination\n");
    return 0;
}  
//#endif	


asmlinkage long sys_ni_syscall(void);

asmlinkage long __arm64_sys_ni_syscall(const struct pt_regs *__unused)
{
	return sys_ni_syscall();
}

/*
 * Wrappers to pass the pt_regs argument.
 */
#define __arm64_sys_personality		__arm64_sys_arm64_personality

#undef __SYSCALL
#define __SYSCALL(nr, sym)	asmlinkage long __arm64_##sym(const struct pt_regs *);
#include <asm/unistd.h>

#undef __SYSCALL
#define __SYSCALL(nr, sym)	[nr] = __arm64_##sym,

const syscall_fn_t sys_call_table[__NR_syscalls] = {
	[0 ... __NR_syscalls - 1] = __arm64_sys_ni_syscall,
#include <asm/unistd.h>
};
