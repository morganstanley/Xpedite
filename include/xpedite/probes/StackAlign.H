///////////////////////////////////////////////////////////////////////////////
//
// Utility asm snippet to align stack prior to calling a method.
//
// Ensures 16 byte stack alignment after storing the current stack pointer 
//
// The counterpart restore stack, as the name implies restores the original
// stack pointer.
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

#define XPEDITE_ALIGN_STACK(REG)     \
    mov   %rsp, %REG;                \
    sub   $16,   %rsp;               \
    and   $-16,  %rsp;               \
    mov   %REG, (%rsp);

#define XPEDITE_RESTORE_STACK mov (%rsp), %rsp;
