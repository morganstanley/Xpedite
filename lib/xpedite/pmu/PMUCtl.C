///////////////////////////////////////////////////////////////////////////////
//
// PMUCtl - Logic to program and collect core, fixed and offcore performance
// counters
//
// Supports programming and collection of pmu events using
// 1. RDPMC with events programmed out of band by xpedite kernel module
//
// Enabling/disabling pmu events, automatically selects appropriate recorders
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#include <xpedite/pmu/PMUCtl.H>
#include <xpedite/probes/RecorderCtl.H>
#include <xpedite/log/Log.H>

namespace xpedite { namespace pmu {

  PmuCtl PmuCtl::_instance;

  PmuCtl::PmuCtl()
    : _genericPmcCount {}, _fixedPmcSet {} {
  }

  void PmuCtl::enableGenericPmc(uint8_t genericPmcCount_) noexcept {
    if(pmcCount() == 0) {
      probes::recorderCtl().activateRecorder(probes::RecorderType::PMC_RECORDER);
    }
    _genericPmcCount = genericPmcCount_;
  }

  void PmuCtl::disableGenericPmc() noexcept {
    if(_genericPmcCount) {
      _genericPmcCount = 0;
      if(pmcCount() == 0) {
        probes::recorderCtl().activateRecorder(probes::RecorderType::EXPANDABLE_RECORDER);
      }
    }
  }

  void PmuCtl::enableFixedPmc(uint8_t index_) noexcept {
    if(pmcCount() == 0) {
      probes::recorderCtl().activateRecorder(probes::RecorderType::PMC_RECORDER);
    }
    _fixedPmcSet.enable(index_);
  }

  void PmuCtl::disableFixedPmc() noexcept {
    if(_fixedPmcSet.size()) {
      _fixedPmcSet.reset();
      if(pmcCount() == 0) {
        probes::recorderCtl().activateRecorder(probes::RecorderType::EXPANDABLE_RECORDER);
      }
    }
  }

  void PmuCtl::disable() noexcept {
    disableGenericPmc();
    disableFixedPmc();
  }

}}
