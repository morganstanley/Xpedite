#######################################################################################
#
# Xpedite Trampoline for recording timestamp and pmu events
#
# Trampoline as the name implies is a piece of code, called by active probes.
# Upon entry, the trampolines provide the following functionalities
# 1. Preservation of register states to preclude any functional side effects
# 2. Checks for sufficient capacity in thread local sample buffers
# 3. Expands storage capacity, if needed
# 4. Records tsc and optionally a set of pmu events
# 5. Returns control back to the call site
#
# Author: Manikandan Dhamodharan, Morgan Stanley
#
#######################################################################################

.section .text

.global  xpediteDefaultTrampoline
.type xpediteDefaultTrampoline, @function 

.global  xpediteTrampoline
.type xpediteTrampoline, @function 

.global  xpediteRecorderTrampoline
.type xpediteRecorderTrampoline, @function 

xpediteDefaultTrampoline:
  ret

# The trampoline code is optimized for ICache footprint
# The common fast path fits in one L2 cache line (< 64 bytes)

xpediteTrampoline:
  push  %rax
  push  %rcx
  push  %rdx
  push  %rsi

  movq  samplesBufferPtr@gottpoff(%rip), %rsi
  movq  samplesBufferEnd@gottpoff(%rip), %rdx
  movq  %fs:(%rsi), %rcx
  cmpq  %fs:(%rdx), %rcx
  jae   1f

  rdtsc
  shl   $0x20, %rdx
  or    %rax, %rdx
  movq  0x20(%rsp), %rax

  movq  %rdx, (%rcx)
  movq  %rax, 0x8(%rcx)
  add   $0x10, %rcx
  movq  %rcx, %fs:(%rsi)

  pop   %rsi
  pop   %rdx
  pop   %rcx
  pop   %rax
  ret

xpediteRecorderTrampoline:
  push  %rax
  push  %rcx
  push  %rdx
  push  %rsi
1:
  push  %rdi
  push  %r8
  push  %r9
  push  %r10
  push  %r11

  rdtsc
  shl    $0x20, %rdx
  or     %rax, %rdx
  mov    %rdx, %rsi
  movq   0x48(%rsp), %rdi

#ifdef XPEDITE_PIE
  callq *activeXpediteRecorder@plt
#else 
  callq *activeXpediteRecorder
#endif
  
  pop  %r11
  pop  %r10
  pop  %r9
  pop  %r8
  pop  %rdi
  pop  %rsi
  pop  %rdx
  pop  %rcx
  pop  %rax
  ret
