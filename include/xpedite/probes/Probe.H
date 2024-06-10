///////////////////////////////////////////////////////////////////////////////
//
// Probes - Probes with near zero overhead, that can be activated at runtime
//
// The probes by default start as 5 byte NOP instructions
// When activated, the NOP's are replace by a JMP instruction, that branches
// to probe specific code for collecting timing and pmc data.
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#pragma once
#include <xpedite/util/Util.H>
#include <xpedite/probes/CallSite.H>
#include <xpedite/probes/RecorderCtl.H>
#include <cstdint>

namespace xpedite { namespace probes {

  namespace test {
    class ProbeTest;
  }

  class ProbeList;

  class Probe
  {
    friend void ::xpediteAddProbe(xpedite::probes::Probe*, xpedite::probes::CallSite, xpedite::probes::CallSite);
    friend void ::xpediteRemoveProbe(xpedite::probes::Probe*);

    friend class ProbeList;
    friend class test::ProbeTest;

    CallSite _callSite;
    void* _trampoline;
    CallSite _recorderCallSite;
    void* _recorderReturnSite;
    Probe* _next;
    Probe* _prev;
    const char* _name;
    const char* _file;
    const char* _func;
    uint32_t _line;
    CallSiteAttr _attr;
    uint32_t _id;

    void activateCallSite() noexcept;

    void deactivateCallSite() noexcept;

    public:

    CallSite callSite() const noexcept { 
      return _callSite;
    }

    const unsigned char* rawCallSite() const noexcept { 
      return const_cast<const unsigned char*>(_callSite->_bytes);
    }

    const unsigned char* rawRecorderCallSite() const noexcept { 
      return const_cast<const unsigned char*>(_recorderCallSite->_bytes);
    }

    void* recorderReturnSite()   const noexcept { return _recorderReturnSite;           }
    const char* name()           const noexcept { return _name;                         }
    const char* file()           const noexcept { return _file;                         }
    const char* func()           const noexcept { return _func;                         }
    Probe* next()                const noexcept { return _next;                         }
    Probe* prev()                const noexcept { return _prev;                         }
    uint32_t line()              const noexcept { return _line;                         }
    uint32_t id()                const noexcept { return _id;                           }
    CallSiteAttr attr()          const noexcept { return _attr;                         }
    bool canStoreData()          const noexcept { return _attr.canStoreData();          }
    bool isActive()              const noexcept { return _attr.isActive();              }
    bool canBeginTxn()           const noexcept { return _attr.canBeginTxn();           }
    bool canSuspendTxn()         const noexcept { return _attr.canSuspendTxn();         }
    bool canResumeTxn()          const noexcept { return _attr.canResumeTxn();          }
    bool canEndTxn()             const noexcept { return _attr.canEndTxn();             }
    bool isPositionIndependent() const noexcept { return _attr.isPositionIndependent(); }

    bool activate() noexcept;

    bool deactivate() noexcept;

    bool isValid(CallSite callSite_, CallSite returnSite_) const noexcept;

    bool matchName(const char* name_) const noexcept;
    bool matchLocation(const char* file_, uint32_t line_) const noexcept;
    bool match(const char* file_, uint32_t line_, const char* name_) const noexcept;

    std::string toString() const;

  } __attribute__((packed,aligned(32)));

}}
