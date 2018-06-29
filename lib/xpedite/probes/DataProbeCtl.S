#######################################################################################
#
# Xpedite Trampoline for recording timestamp, data and pmu events
#
# Trampoline as the name implies is a piece of code, called by active probes.
# Upon entry, the trampolines provide the following functionalities
# 1. Preservation of register states to preclude any functional side effects
# 2. Checks for sufficient capacity in thread local sample buffers
# 3. Expands storage capacity, if needed
# 4. Records tsc, 16 bytes of integral data and optionally a set of pmu events
# 5. Returns control back to the call site
#
# Author: Manikandan Dhamodharan, Morgan Stanley
#
#######################################################################################

.section .text
.global  xpediteDataProbeTrampoline
.type xpediteDataProbeTrampoline, @function 

.global  xpediteDataProbeRecorderTrampoline
.type xpediteDataProbeRecorderTrampoline, @function 

xpediteDataProbeTrampoline:
  push  %rax
  push  %rcx
  push  %rdx
  push  %rsi
  push  %rdi

  movq  samplesBufferPtr@gottpoff(%rip), %rsi
  movq  samplesBufferEnd@gottpoff(%rip), %rdi
  movq  %fs:(%rsi), %rcx
  cmpq  %fs:(%rdi), %rcx
  jae   1f

  movq   %rax, 0x10(%rcx)
  movq   %rdx, 0x18(%rcx)

  rdtsc
  orq   $0x40000000, %rdx
  shl   $0x20, %rdx
  or    %rax, %rdx
  movq  0x28(%rsp), %rax

  movq   %rdx, (%rcx)
  movq   %rax, 0x8(%rcx)
  add    $0x20, %rcx
  movq   %rcx, %fs:(%rsi)

  pop   %rdi
  pop   %rsi
  pop   %rdx
  pop   %rcx
  pop   %rax
  ret

xpediteDataProbeRecorderTrampoline:
  push  %rax
  push  %rcx
  push  %rdx
  push  %rsi
  push  %rdi
1:
  push  %r8
  push  %r9
  push  %r10
  push  %r11

  movq   %rax, %r8
  movq   %rdx, %rcx

  rdtsc
  shl    $0x20, %rdx
  or     %rax, %rdx
  mov    %rdx, %rsi
  movq   0x48(%rsp), %rdi
  movq   %r8, %rdx

  callq *activeXpediteDataProbeRecorder

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
