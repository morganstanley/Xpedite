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

#include <xpedite/probes/Probe.H>
#include <cstdint>
#include <sstream>

namespace xpedite { namespace probes {

  void Probe::activateCallSite() noexcept {
    Instructions instructions {_callSite->_quadWord};
    if(isPositionIndependent()) {
      memcpy(instructions._bytes, PIC_CALL, sizeof(PIC_CALL));
      XpediteLogInfo << "Enable position independent probe " << toString() << " | with indirect jump" << XpediteLogEnd;
    }
    else {
      instructions._bytes[0] = OPCODE_CALL;
      Trampoline trampoline;
      if(canStoreData()) {
        trampoline = (recorderCtl().pmcCount() ? xpediteDataProbeRecorderTrampoline : xpediteDataProbeTrampoline);
      }
      else if(canSuspendTxn()) {
        trampoline = (recorderCtl().pmcCount() ? xpediteIdentityRecorderTrampoline : xpediteIdentityTrampoline);
      }
      else {
        trampoline = (recorderCtl().pmcCount() ? xpediteRecorderTrampoline : xpediteTrampoline);
      }
      uint32_t jmpOffset {offset(_callSite, trampoline)};
      memcpy(instructions._bytes + 1, &jmpOffset, sizeof(jmpOffset));
      XpediteLogInfo << "Enable probe " << toString() << " | trampoline - " << reinterpret_cast<void*>(trampoline)
        << " offset - " << jmpOffset << XpediteLogEnd;
    }
    _callSite->_quadWord = instructions._quadWord;
  }

  void Probe::deactivateCallSite() noexcept {
    Instructions instructions {_callSite->_quadWord};
    memcpy(instructions._bytes, FIVE_BYTE_NOP, sizeof(FIVE_BYTE_NOP));
    _callSite->_quadWord = instructions._quadWord;
  }

  bool Probe::isValid(CallSite callSite_, CallSite returnSite_) const noexcept {
    auto callSiteLen = offset(returnSite_, callSite_);
    if(callSiteLen != CAll_SITE_LEN) {
      fprintf(stderr, "detected probe ['%s' at %s:%d] with invalid call site size (%ld bytes)"
        " call site addesses (%p) | return addesses (%p)\n", _name, _file, _line, callSiteLen, callSite_, returnSite_);
      return {};
    }

    if(!callSite_ || callSite_ != _callSite) {
      fprintf(stderr, "detected probe ['%s' at %s:%d] with mismatching call site addresses (%p) expected (%p)\n",
             _name, _file, _line, callSite_, returnSite_);
      return {};
    }

    if(reinterpret_cast<uintptr_t>(_callSite) % 8) {
      fprintf(stderr, "detected probe probe ['%s' at %s:%d] with unaligned call site %p - expected 8 byte alignment\n",
             _name, _file, _line, callSite_);
      return {};
    }

    if(memcmp(&FIVE_BYTE_NOP, rawCallSite(), sizeof(FIVE_BYTE_NOP))) {
      fprintf(stderr, "detected probe ['%s' at %s:%d] with invalid opcode at call site - expected 5 byte NOP"
          " found %2X %2X %2X %2X %2X\n", _name, _file, _line, rawCallSite()[0], rawCallSite()[1], rawCallSite()[2]
          , rawCallSite()[3], rawCallSite()[4]);
      return {};
    }
    return true;
  }

  bool Probe::match(const char* file_, uint32_t line_, const char* name_) const noexcept {
    if(name_ == _name || (name_ && !strcmp(_name, name_))) {
      return true;
    }

    if(file_ && strstr(_file, file_) && (!line_ || _line == line_)) {
      return true;
    }
    return {};
  }

  std::string Probe::toString() const {
    std::ostringstream os;
    os << "Probe [" << _name << std::hex << " - " << this << "]"
      << " call site - " << reinterpret_cast<const void*>(rawCallSite())
      << std::dec << " at - " << _file << ":" << _line;
    return os.str();
  }
}}
