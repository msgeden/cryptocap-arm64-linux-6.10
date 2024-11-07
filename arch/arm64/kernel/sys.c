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
SYSCALL_DEFINE0(ccall)
{
	volatile uint64_t ttbr0, elr, spsr, sp_el0_current; 

	asm volatile ("mrs %0, ttbr0_el1" : "=r"(current->saved_ttbr0_el1));
    asm volatile ("mrs %0, elr_el1" : "=r"(current->saved_elr_el1));
    asm volatile ("mrs %0, spsr_el1" : "=r"(current->saved_spsr_el1));
    asm volatile ("mrs %0, sp_el0" : "=r"(current->saved_sp_el0));

	ttbr0 = current->saved_ttbr0_el1;
    elr = current->saved_elr_el1;
    spsr = current->saved_spsr_el1;
    sp_el0_current=current->saved_sp_el0;

	// Set user SP using CLC.SP as the callee's SP
    //asm volatile (".word 0x03700049"); //clsp #0, x9
    //asm volatile ("msr	sp_el0, x9");
 
    // Reset condition flags in spsr1_el1
    //mrs	x9, spsr_el1
    //and	x9, x9, #0xfffffff
    //msr	spsr_el1, x9

    // Set elr_el1 using CLC.PC for the address to be jumped
    asm volatile (".word 0x03600049"); //clpc #0, x9

    asm volatile ("msr	elr_el1, x9");
  
    // Set ttbr0_el1 using CLC.PT for the address space to be jumped
    asm volatile (".word 0x03800049"); //clpt #0, x9
    asm volatile ("msr	ttbr0_el1, x9");

    //(Instruction Synchronization Barrier)
	asm volatile ("isb");
    //https://developer.arm.com/documentation/ddi0488/c/system-control/aarch64-register-summary/aarch64-tlb-maintenance-operations
    asm volatile ("tlbi vmalle1");
    //(Data Synchronization Barrier - Full System)
    asm volatile ("dsb sy");
	
	asm volatile ("eret");

	return 0;
}
SYSCALL_DEFINE0(cret)
{
	volatile uint64_t ttbr0, elr, spsr, sp_el0_current;
	
    // Retrieve system registers from task_struct
    sp_el0_current = current->saved_sp_el0;
    spsr = current->saved_spsr_el1;
    elr = current->saved_elr_el1;
	ttbr0 = current->saved_ttbr0_el1;
    
	// Set the system registers with the retrieved values
    // asm volatile ("msr spsr_el1, %0" : : "r"(spsr));   // Set SPSR
    asm volatile ("msr elr_el1, %0" : : "r"(elr));     // Set ELR
    asm volatile ("msr ttbr0_el1, %0" : : "r"(ttbr0)); // Set TTBR0
    
    //(Instruction Synchronization Barrier)
	asm volatile ("isb");
    //https://developer.arm.com/documentation/ddi0488/c/system-control/aarch64-register-summary/aarch64-tlb-maintenance-operations
    asm volatile ("tlbi vmalle1");
    //(Data Synchronization Barrier - Full System)
    asm volatile ("dsb sy");
	
	asm volatile ("eret");

	return 0;
}

static DEFINE_SPINLOCK(return_value_lock);  // Lock for thread safety
static DECLARE_COMPLETION(pcall_done);      // Completion variable to signal `pret` completion
// Track the original caller for switching back in pret
struct caller_data {
    struct task_struct *caller_task; // Original caller process
    pid_t caller_pid;                // PID of the original caller
    uint64_t ret_val;              // Return value to be returned to the original caller
};
static struct caller_data saved_caller;
SYSCALL_DEFINE2(pcall, pid_t, target_pid, uint64_t, target_pc) {
    
    struct task_struct *target_task;
	struct pt_regs *regs;
    
    printk(KERN_INFO "pcall entry: target_pid:%ld, target_pc:%ld\n", target_pid, target_pc);

    // Get the task struct of the target process
    target_task = find_task_by_vpid(target_pid);
    if (!target_task){
        printk(KERN_ERR "pccall error: Target task not found.\n");
        return -ESRCH;  // Return error if target process does not exist
    }

    // Save the original caller task and PID for later use
    spin_lock(&return_value_lock);
    saved_caller.caller_task = current;
    saved_caller.caller_pid = task_pid_nr(current);
    spin_unlock(&return_value_lock);

    // Set the target process's PC to the function address
    regs = task_pt_regs(target_task);
    regs->pc = target_pc;

    //set_current_state(TASK_INTERRUPTIBLE);
    
    // Wake up the callee task
    wake_up_process(target_task);

    // Wait until `pret` signals completion
    wait_for_completion(&pcall_done);
    //schedule();  // Yield control

    printk(KERN_INFO "pcall termination\n");

    return saved_caller.ret_val;  // Return the value set by `pret`
}

SYSCALL_DEFINE1(pret, uint64_t, ret_val) {

    struct pt_regs *regs;
    
    printk(KERN_INFO "pret entry: caller pid:%d, ret_val:%ld\n", saved_caller.caller_pid, ret_val);
    
    // Ensure that pret is being called by the target process, not the caller
    if (task_pid_nr(current) == saved_caller.caller_pid) {
        return -EPERM;  // Return error if called by the original caller
    }

    // Switch back to the original caller
    spin_lock(&return_value_lock);
    if (saved_caller.caller_task) {
        // Set the return value in the original caller's register
        regs = task_pt_regs(saved_caller.caller_task);
        regs->regs[0] = ret_val;  // Set x0 to ret_val for the original caller
        saved_caller.ret_val = ret_val;  // Save the return value

        wake_up_process(saved_caller.caller_task);  // Wake up the original caller
        saved_caller.caller_task = NULL;
        saved_caller.caller_pid = 0;
    }
    spin_unlock(&return_value_lock);

    // Signal `pcall` that `pret` is complete

    complete(&pcall_done);

    printk(KERN_INFO "pret termination\n");
    
    return ret_val;
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
