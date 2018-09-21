///////////////////////////////////////////////////////////////////////////////
//
// Xpedite Probes - Probes with near zero overhead, that can be activated at runtime
//
// The probes by default start as 5 byte NOP instructions
// When activated, the NOP's are replace by a JMP instruction, that branches
// to probe specific code for collecting timing and pmc data.
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
