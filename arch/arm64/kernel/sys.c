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

// //#ifdef TARGET_CRYPTO_CAP
// SYSCALL_DEFINE1(ccall, unsigned long, variable)
// {
// 	printk(KERN_DEBUG "Debug: CCALL variable %d\n", variable);
// 	// Ensure the user has the necessary privileges to change the TTBR
//     if (!capable(CAP_SYS_ADMIN)) {
//         return -EPERM;  // Return permission error
//     }

//     // Ensure the new TTBR value is valid
//     if (!variable) {
//         return -EINVAL;  // Return invalid argument error
//     }

//     // Change the TTBR1_EL1 (or TTBR0_EL1 if you want to change user-space)
//     asm volatile (
//         "msr ttbr0_el1, %0\n"  // Write the new TTBR value to TTBR0_EL1
//         "dsb ish\n"            // Data synchronization barrier
//         "isb\n"                // Instruction synchronization barrier
//         : : "r" (variable) : "memory"
//     );

//     // Optionally flush the TLB to apply changes
//     // asm volatile("tlbi vmalle1is\n");
// 	// asm volatile("dsb ish\n");
// 	// asm volatile("isb\n");
//     // asm volatile("tlbi vmalle1is\; dsb ish\; isb\;");
//     return 0;  // Success
// }syscall_table.S
// //endif 

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
