///////////////////////////////////////////////////////////////////////////////
//
// Global static definitions for Per thread probe sample buffers
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#include <xpedite/platform/Builtins.H>
#include <xpedite/framework/SamplesBuffer.H>
#include <xpedite/util/Util.H>

static __thread xpedite::framework::SamplesBuffer* _tlSamplesBuffer;

__thread xpedite::probes::Sample* samplesBufferPtr;
__thread xpedite::probes::Sample* samplesBufferEnd;

namespace xpedite { namespace framework {

  alignas(common::ALIGNMENT) std::atomic<SamplesBuffer*> SamplesBuffer::_head {};

  bool SamplesBuffer::isInitialized() {
    return _tlSamplesBuffer != nullptr;
  }

  SamplesBuffer* SamplesBuffer::samplesBuffer() {
    if(XPEDITE_UNLIKELY(!_tlSamplesBuffer)) {
      _tlSamplesBuffer = SamplesBuffer::allocate();
    }
    return _tlSamplesBuffer;
  }

  void SamplesBuffer::expand() {
    if(probes::config().verbose()) {
      XpediteLogInfo << "Xpedite SamplesBuffer expand: tid - " << util::gettid() << " | begin - " << samplesBufferPtr
        << " | end - " << samplesBufferEnd << XpediteLogEnd;
    }
    std::tie(samplesBufferPtr, samplesBufferEnd) = samplesBuffer()->nextWritableRange();
  }

}}

