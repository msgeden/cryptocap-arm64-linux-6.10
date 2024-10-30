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
SYSCALL_DEFINE0(cdummy)
{
	int ret=0;
	printk(KERN_INFO "cdummy is called!");
    asm volatile(
        "mov %0, #54\n\t"       
		:"=r"(ret)                   
		:
		:
	);
	return ret;
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
    asm volatile ("msr spsr_el1, %0" : : "r"(spsr));   // Set SPSR
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
