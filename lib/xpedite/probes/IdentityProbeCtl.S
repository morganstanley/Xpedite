#######################################################################################
#
# Xpedite Identity probe trampoline for recording timestamp and pmu events
#
# Trampoline as the name implies is a piece of code, called by active probes.
# Upon entry, the trampolines provide the following functionalities
# 1. Preservation of register states to preclude any functional side effects
# 2. Checks for sufficient capacity in thread local sample buffers
# 3. Expands storage capacity, if needed
# 4. Records tsc and optionally a set of pmu events
# 5. Returns control back to the call site with unqiue txn id in %rax:%rdx
#
# Author: Manikandan Dhamodharan, Morgan Stanley
#
#######################################################################################

.section .text
.global  xpediteIdentityTrampoline
.type xpediteIdentityTrampoline, @function 

.global  xpediteIdentityRecorderTrampoline
.type xpediteIdentityRecorderTrampoline, @function 

# The trampoline code is optimized for ICache footprint
# The common fast path fits in one L2 cache line (< 64 bytes)

xpediteIdentityTrampoline:
  push  %rcx

  movq  samplesBufferPtr@gottpoff(%rip), %rax
  movq  samplesBufferEnd@gottpoff(%rip), %rdx
  movq  %fs:(%rax), %rcx
  cmpq  %fs:(%rdx), %rcx
  jae   1f
  addq  $0x10, %fs:(%rax)

  rdtsc
  shl   $0x20, %rdx
  or    %rax, %rdx
  movq  %rdx, (%rcx)
  movq  0x08(%rsp), %rax
  movq  %rax, 0x8(%rcx)
  movq  %fs:0, %rax

  pop   %rcx
  ret

xpediteIdentityRecorderTrampoline:
  push  %rcx
1:
  push  %rsi
  push  %rdi
  push  %r8
  push  %r9
  push  %r10
  push  %r11

  rdtsc
  shl    $0x20, %rdx
  or     %rax, %rdx
  mov    %rdx, %rsi
  movq   0x38(%rsp), %rdi

  push  %rdx
#ifdef XPEDITE_PIE
  callq *activeXpediteRecorder@plt
#else 
  callq *activeXpediteRecorder
#endif
  pop  %rdx

  pop  %r11
  pop  %r10
  pop  %r9
  pop  %r8
  pop  %rdi
  pop  %rsi
  pop  %rcx
  movq  %fs:0, %rax
  ret
