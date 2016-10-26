/*
 * Copyright (c) 2016 Wuklab, Purdue University. All rights reserved.
 * Definitions for 16-bit setup code.
 */

#ifndef _ASM_X86_SETUP_H_
#define _ASM_X86_SETUP_H_

#define COMMAND_LINE_SIZE	2048

#define OLD_CL_MAGIC		0xA33F
#define OLD_CL_ADDRESS		0x020	/* Relative to real mode data */
#define NEW_CL_POINTER		0x228	/* Relative to real mode data */

#endif /* _ASM_X86_SETUP_H_ */