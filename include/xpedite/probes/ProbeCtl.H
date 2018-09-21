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
#include <xpedite/platform/Builtins.H>
#include <xpedite/probes/CallSite.H>

namespace xpedite { namespace probes {

  enum class Command
  {
    ENABLE  = 1,
    DISABLE = 2,
    REPORT  = 3
  };

  void probeCtl(Command cmd_, const char* file_, int line_, const char* name_);

}}

extern "C" {
  void XPEDITE_CALLBACK xpediteAddProbe(xpedite::probes::Probe* probe_, xpedite::probes::CallSite callSite_, xpedite::probes::CallSite returnSite_);
  void XPEDITE_CALLBACK xpediteRemoveProbe(xpedite::probes::Probe* probe_);
}

extern xpedite::probes::Trampoline xpediteTrampolinePtr;
extern xpedite::probes::Trampoline xpediteDataProbeTrampolinePtr;
extern xpedite::probes::Trampoline xpediteIdentityTrampolinePtr;

#define XPEDITE_ALIGN_STACK                                      \
    "   mov   %%rsp, %%rdi        \n"                            \
    "   sub   $16,   %%rsp        \n"                            \
    "   and   $-16,  %%rsp        \n"                            \
    "   mov   %%rdi, (%%rsp)      \n"

#define XPEDITE_RESTORE_STACK "   mov   (%%rsp), %%rsp      \n"

#define XPEDITE_PROBE_ASM(XPEDITE_TRAMPOLINE)                    \
    ".align 8                     \n"                            \
    "1:"                                                         \
    ".byte 0x0F, 0x1F, 0x44, 0x00, 0x00 \n"                      \
    ".pushsection .xpeditedata.rel,\"wa?\",@progbits \n"         \
    ".align 32                    \n"                            \
    "2: .quad 1b                  \n"                            \
    "   .quad 3f                  \n"                            \
    "   .quad 4f                  \n"                            \
    "   .quad 5f                  \n"                            \
    "   .quad 0                   \n"                            \
    "   .quad 0                   \n"                            \
    "   .quad %P[Name]            \n"                            \
    "   .quad %P[File]            \n"                            \
    "   .quad %P[Func]            \n"                            \
    "   .long %P[Line]            \n"                            \
    "   .long %P[Attributes]      \n"                            \
    "   .long 0                   \n"                            \
    ".popsection                  \n"                            \
    ".pushsection .xpeditecode.rel,\"xa?\",@progbits \n"         \
    "3:"                                                         \
    "   sub   $152, %%rsp         \n"                            \
    "   push  %%rcx                \n"                           \
    "   .align 8                  \n"                            \
    "4:"                                                         \
    "   mov " #XPEDITE_TRAMPOLINE "@GOTPCREL(%%rip), %%rcx \n"   \
    "   callq *(%%rcx)             \n"                           \
    "5:"                                                         \
    "   pop  %%rcx                 \n"                           \
    "   add   $152, %%rsp         \n"                            \
    "   jmp 8f                    \n"                            \
    "6:"                                                         \
    XPEDITE_ALIGN_STACK                                          \
    "   leaq  2b(%%rip), %%rdi    \n"                            \
    "   leaq  1b(%%rip), %%rsi    \n"                            \
    "   leaq  8f(%%rip), %%rdx    \n"                            \
    "   call  xpediteAddProbe@plt \n"                            \
    XPEDITE_RESTORE_STACK                                        \
    "   ret                       \n"                            \
    "7:"                                                         \
    XPEDITE_ALIGN_STACK                                          \
    "   leaq  2b(%%rip),%%rdi    \n"                             \
    "   call  xpediteRemoveProbe@plt \n"                         \
    XPEDITE_RESTORE_STACK                                        \
    "   ret                       \n"                            \
    ".popsection                  \n"                            \
    ".pushsection .init_array, \"?\" \n"                         \
    ".align 8                     \n"                            \
    "  .quad 6b                   \n"                            \
    ".popsection                  \n"                            \
    ".pushsection .fini_array, \"?\" \n"                         \
    ".align 8                     \n"                            \
    "  .quad 7b                   \n"                            \
    ".popsection                  \n"                            \
    "8:\n"                                                       \

#define XPEDITE_DEFINE_PROBE(NAME, FILE, LINE, FUNC, ATTRIBUTES) \
  asm __volatile__ (                                             \
    XPEDITE_PROBE_ASM(xpediteTrampolinePtr)                      \
    ::                                                           \
     [Name] "i"(NAME),                                           \
     [File] "i"(FILE),                                           \
     [Func] "i"(FUNC),                                           \
     [Line] "i"(LINE),                                           \
     [Attributes] "i"(ATTRIBUTES)                                \
    : "flags")

#define XPEDITE_FLAGGED_PROBE(NAME, ATTRIBUTES) XPEDITE_DEFINE_PROBE(#NAME, __FILE__, __LINE__, __PRETTY_FUNCTION__, ATTRIBUTES)

#define XPEDITE_DEFINE_DATA_PROBE(NAME, DATA, FILE, LINE, FUNC, ATTRIBUTES)                   \
  asm __volatile__ (                                                                          \
    XPEDITE_PROBE_ASM(xpediteDataProbeTrampolinePtr)                                          \
    ::                                                                                        \
     [Name] "i"(NAME),                                                                        \
     [File] "i"(FILE),                                                                        \
     [Func] "i"(FUNC),                                                                        \
     [Line] "i"(LINE),                                                                        \
     [Attributes] "i"(ATTRIBUTES | xpedite::probes::CallSiteAttr::CAN_STORE_DATA),            \
     "A"(DATA)                                                                                \
    : "flags")

#define XPEDITE_FLAGGED_DATA_PROBE(NAME, DATA, ATTRIBUTES)   XPEDITE_DEFINE_DATA_PROBE(#NAME, DATA, __FILE__, __LINE__, __PRETTY_FUNCTION__, ATTRIBUTES)

#define XPEDITE_DEFINE_IDENTITY_PROBE(NAME, FILE, LINE, FUNC, ATTRIBUTES) \
  ({ __uint128_t id {};                                                   \
    asm __volatile__ (                                                    \
      XPEDITE_PROBE_ASM(xpediteIdentityTrampolinePtr)                     \
      : "=A"(id):                                                         \
      [Name] "i"(NAME),                                                   \
      [File] "i"(FILE),                                                   \
      [Func] "i"(FUNC),                                                   \
      [Line] "i"(LINE),                                                   \
      [Attributes] "i"(ATTRIBUTES)                                        \
      : "flags");                                                         \
    id;})

#define XPEDITE_FLAGGED_IDENTITY_PROBE(NAME, FLAGS) XPEDITE_DEFINE_IDENTITY_PROBE(#NAME, __FILE__, __LINE__, __PRETTY_FUNCTION__, FLAGS)
