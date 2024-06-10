///////////////////////////////////////////////////////////////////////////////
//
// CallSite - A instrumented location in instruction stream, that can call 
// a trampoline upon activation.
//
// Activated call sites jump to pre-determined trampoline location to collect 
// samples for a profile session
// 
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#pragma once
#include <sstream>
#include <cstdint>

namespace xpedite { namespace probes {

  namespace test {
    class ProbeTest;
  }

  class Probe;

  static constexpr unsigned CAll_SITE_LEN {5};
  static constexpr uint8_t OPCODE_JMP {0xE9};
  static constexpr uint8_t OPCODE_CALL {0xE8};
  static constexpr uint8_t OPCODE_CALL_INDIRECT {0xFF};
  static constexpr uint8_t OPCODE_NOP {0x90};
  static constexpr unsigned char FIVE_BYTE_NOP [CAll_SITE_LEN] {0x0F, 0x1F, 0x44, 0x00, 0x00};
  static constexpr unsigned char PIC_CALL [CAll_SITE_LEN] {OPCODE_NOP, OPCODE_NOP, OPCODE_NOP, OPCODE_CALL_INDIRECT, 0xD1};

  class CallSiteAttr
  {
    friend class test::ProbeTest;

    uint32_t _attr;

    public:

    enum
    {
      IS_ACTIVE               = 1 << 0,
      CAN_BEGIN_TXN           = 1 << 1,
      CAN_SUSPEND_TXN         = 1 << 2,
      CAN_RESUME_TXN          = 1 << 3,
      CAN_END_TXN             = 1 << 4,
      CAN_STORE_DATA          = 1 << 5,
      IS_POSITION_INDEPENDENT = 1 << 6
    };

    void markActive() noexcept { 
      _attr |= IS_ACTIVE;
    }

    void markInActive() noexcept { 
      _attr &= ~IS_ACTIVE;
    }

    bool isActive()              const noexcept { return _attr & IS_ACTIVE;               }
    bool canStoreData()          const noexcept { return _attr & CAN_STORE_DATA;          }
    bool canBeginTxn()           const noexcept { return _attr & CAN_BEGIN_TXN;           }
    bool canSuspendTxn()         const noexcept { return _attr & CAN_SUSPEND_TXN;         }
    bool canResumeTxn()          const noexcept { return _attr & CAN_RESUME_TXN;          }
    bool canEndTxn()             const noexcept { return _attr & CAN_END_TXN;             }
    bool isPositionIndependent() const noexcept { return _attr & IS_POSITION_INDEPENDENT; }

    std::string toString() const {
      std::ostringstream os;
      const char* attr[16] {};
      int count {};

      if(canStoreData())  { attr[count++] = "canStoreData";  }
      if(canBeginTxn())   { attr[count++] = "canBeginTxn";   }
      if(canSuspendTxn()) { attr[count++] = "canSuspendTxn"; }
      if(canResumeTxn())  { attr[count++] = "canResumeTxn";  }
      if(canEndTxn())     { attr[count++] = "canEndTxn";     }

      if(count) {
        os << attr[0];
        for(unsigned i=1; i<sizeof(attr)/sizeof(attr[0]) && attr[i]; ++i) {
          os << "," << attr[i];
        }
      }
      else {
        os << "None";
      }
      return os.str();
    }
  } __attribute__((packed));

  union Instructions
  {
    Instructions(uint64_t quadWord_)
      : _quadWord {quadWord_} {
    }

    uint64_t _quadWord;
    unsigned char _bytes[sizeof(uint64_t)];
  };

  using CallSite = volatile Instructions*; 

  using Trampoline = void (*) (); 

  inline uint32_t offset(CallSite callSite_, Trampoline trampoline_) noexcept {
    return reinterpret_cast<unsigned char*>(trampoline_) - reinterpret_cast<volatile unsigned char*>(callSite_) - CAll_SITE_LEN;
  }

  inline long offset(CallSite lhs_, CallSite rhs_) noexcept {
    return reinterpret_cast<volatile unsigned char*>(lhs_) - reinterpret_cast<volatile unsigned char*>(rhs_);
  }

  inline uint32_t offset(CallSite callSite_, void* trampoline_) noexcept {
    return reinterpret_cast<unsigned char*>(trampoline_) - reinterpret_cast<volatile unsigned char*>(callSite_) - CAll_SITE_LEN;
  }

  inline const void* getcallSite(const void* returnSite_) {
    return reinterpret_cast<const char*>(returnSite_) - CAll_SITE_LEN;
  }

}}

extern "C" {
  void xpediteAddProbe(xpedite::probes::Probe*, xpedite::probes::CallSite, xpedite::probes::CallSite);
  void xpediteRemoveProbe(xpedite::probes::Probe*);
  void xpediteDefaultTrampoline();
  void xpediteTrampoline();
  void xpediteRecorderTrampoline();
  void xpediteDataProbeTrampoline();
  void xpediteDataProbeRecorderTrampoline();
  void xpediteIdentityTrampoline();
  void xpediteIdentityRecorderTrampoline();
}

namespace std {
  template <>
  struct hash<const xpedite::probes::CallSite>
  {
    size_t operator()(const xpedite::probes::CallSite callSite_) const {
      return reinterpret_cast<size_t>(const_cast<const unsigned char*>(callSite_->_bytes));
    }
  };
}

