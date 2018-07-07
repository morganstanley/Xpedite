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

extern void* xpediteTrampolinePtr;
extern void* xpediteDataProbeTrampolinePtr;
extern void* xpediteIdentityTrampolinePtr;

#define XPEDITE_PROBE_ASM                                     \
    ".align 8                     \n"                         \
    "1:"                                                      \
    ".byte 0x0F, 0x1F, 0x44, 0x00, 0x00 \n"                   \
    ".pushsection .xpeditedata.rel,\"wa?\",@progbits \n"      \
    ".align 32                    \n"                         \
    "2: .quad 1b                  \n"                         \
    "   .quad 0                   \n"                         \
    "   .quad 0                   \n"                         \
    "   .quad %P[Name]            \n"                         \
    "   .quad %P[File]            \n"                         \
    "   .quad %P[Func]            \n"                         \
    "   .long %P[Line]            \n"                         \
    "   .long %P[Attributes]      \n"                         \
    "   .long 0                   \n"                         \
    ".popsection                  \n"                         \
    ".pushsection .xpeditecode.rel,\"xa?\",@progbits \n"      \
    "3:"                                                      \
    "   leaq  2b(%%rip), %%rdi    \n"                         \
    "   leaq  1b(%%rip), %%rsi    \n"                         \
    "   leaq  5f(%%rip), %%rdx    \n"                         \
    "   call  xpediteAddProbe@plt \n"                         \
    "   ret                       \n"                         \
    "4:  leaq  2b(%%rip),%%rdi    \n"                         \
    "   call  xpediteRemoveProbe@plt \n"                      \
    "   ret                       \n"                         \
    ".popsection                  \n"                         \
    ".pushsection .init_array, \"?\" \n"                      \
    ".align 8                     \n"                         \
    "  .quad 3b                   \n"                         \
    ".popsection                  \n"                         \
    ".pushsection .fini_array, \"?\" \n"                      \
    ".align 8                     \n"                         \
    "  .quad 4b                   \n"                         \
    ".popsection                  \n"                         \
    "5:\n"                                                    \

#ifdef XPEDITE_PIC
#define XPEDITE_TRAMPOLINE "c"(xpediteTrampolinePtr),
#define XPEDITE_DATA_PROBE_TRAMPOLINE "c"(xpediteDataProbeTrampolinePtr),
#define XPEDITE_IDENTITY_TRAMPOLINE "c"(xpediteIdentityTrampolinePtr),
#define ATTR_PIC xpedite::probes::CallSiteAttr::IS_POSITION_INDEPENDENT
#else 
#define XPEDITE_TRAMPOLINE
#define XPEDITE_DATA_PROBE_TRAMPOLINE
#define XPEDITE_IDENTITY_TRAMPOLINE
#define ATTR_PIC 0
#endif

#define XPEDITE_DEFINE_PROBE(NAME, FILE, LINE, FUNC, ATTRIBUTES) \
  asm __volatile__ (                                             \
    XPEDITE_PROBE_ASM                                            \
    ::                                                           \
     XPEDITE_TRAMPOLINE                                          \
     [Name] "i"(NAME),                                           \
     [File] "i"(FILE),                                           \
     [Func] "i"(FUNC),                                           \
     [Line] "i"(LINE),                                           \
     [Attributes] "i"(ATTR_PIC | ATTRIBUTES)                     \
    : "flags")

#define XPEDITE_FLAGGED_PROBE(NAME, ATTRIBUTES) XPEDITE_DEFINE_PROBE(#NAME, __FILE__, __LINE__, __PRETTY_FUNCTION__, ATTRIBUTES)

#define XPEDITE_DEFINE_DATA_PROBE(NAME, DATA, FILE, LINE, FUNC, ATTRIBUTES)                     \
  ({ __uint128_t data = DATA;                                                                   \
    asm __volatile__ (                                                                          \
      XPEDITE_PROBE_ASM                                                                         \
      ::                                                                                        \
       XPEDITE_DATA_PROBE_TRAMPOLINE                                                            \
       [Name] "i"(NAME),                                                                        \
       [File] "i"(FILE),                                                                        \
       [Func] "i"(FUNC),                                                                        \
       [Line] "i"(LINE),                                                                        \
       [Attributes] "i"(ATTR_PIC | ATTRIBUTES | xpedite::probes::CallSiteAttr::CAN_STORE_DATA), \
       "A"(data)                                                                                \
      : "flags"); })

#define XPEDITE_FLAGGED_DATA_PROBE(NAME, DATA, ATTRIBUTES)   XPEDITE_DEFINE_DATA_PROBE(#NAME, DATA, __FILE__, __LINE__, __PRETTY_FUNCTION__, ATTRIBUTES)

#define XPEDITE_DEFINE_IDENTITY_PROBE(NAME, FILE, LINE, FUNC, ATTRIBUTES) \
  ({ __uint128_t id {};                                                   \
    asm __volatile__ (                                                    \
      XPEDITE_PROBE_ASM                                                   \
      : "=A"(id):                                                         \
      XPEDITE_IDENTITY_TRAMPOLINE                                         \
      [Name] "i"(NAME),                                                   \
      [File] "i"(FILE),                                                   \
      [Func] "i"(FUNC),                                                   \
      [Line] "i"(LINE),                                                   \
      [Attributes] "i"(ATTR_PIC | ATTRIBUTES)                             \
      : "flags");                                                         \
    id;})

#define XPEDITE_FLAGGED_IDENTITY_PROBE(NAME, FLAGS) XPEDITE_DEFINE_IDENTITY_PROBE(#NAME, __FILE__, __LINE__, __PRETTY_FUNCTION__, FLAGS)
