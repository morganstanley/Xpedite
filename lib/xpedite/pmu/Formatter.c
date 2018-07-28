///////////////////////////////////////////////////////////////////////////////////////////////
//
// Logic to format pmu request and event objects to strings
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////////////////////

#include <xpedite/pmu/Formatter.h>

void logRequest(unsigned ctrIndex_, const PMUGpEvent* e_, uint32_t b_) {
  XPEDITE_LOG("eventSelect = 0x%02X | unitMask = 0x%02X | user = 0x%02X | kernel = 0x%02X |"
    " invertCMask = 0x%02X | counterMask = 0x%02X | -> PerfEvtSel%u [0x%08llX]\n", 
    0xff & e_->_eventSelect, 
    0xff & e_->_unitMask, 
    0xff & e_->_user, 
    0xff & e_->_kernel, 
    0xff & e_->_invertCMask, 
    0xff & e_->_counterMask,
    ctrIndex_,
    (uint64_t)b_
  );
}

void logOffcoreRequest(unsigned ctrIndex_, uint64_t e_) {
  XPEDITE_LOG("setting MSR_OFFCORE_RSP_%u -> %llx\n", ctrIndex_, (long long unsigned int) e_);
}

static unsigned char toBoolenChar(int v_) {
  return v_? 'y' : 'n';
}

int gpEventToString(const PMUGpEvent* event_, char* buffer_, int size_) {
  return snprintf(buffer_, size_,
    "\nGpEvent {eventSelect - %2x, unitMask - %2x, user - %c, kernel - %c"
    ", invertCMask - %2x, counterMask - %2x, edgeDetect - %2x, anyThread - %c}",
    0xff & event_->_eventSelect,
    0xff & event_->_unitMask,
    toBoolenChar(event_->_user),
    toBoolenChar(event_->_kernel),
    0xff & event_->_invertCMask,
    0xff & event_->_counterMask,
    0xff & event_->_edgeDetect,
    toBoolenChar(event_->_anyThread)
  );
}

int fixedEventToString(const PMUFixedEvent* event_, char* buffer_, int size_) {
  return snprintf(buffer_, size_, "FixedEvent {index - %2d, user - %c, kernel - %c"
      , (int) event_->_ctrIndex, toBoolenChar(event_->_user), toBoolenChar(event_->_kernel));
}

int offcoreEventToString(const PMUOffcoreEvent* event_, char* buffer_, int size_) {
  return snprintf(buffer_, size_, "OffcoreEvent {index - %llx", *event_);
}

int pmcrqToString(const PMUCtlRequest* request_, char* buffer_, int size_) {

  int i=0;
  buffer_[0] = '\0';
  char* buffer = buffer_;
  int capacity = size_;
  int fmtSize = 0;

  for(; capacity && i< XPEDITE_PMC_CTRL_GP_EVENT_MAX; ++i, buffer += fmtSize, capacity -= fmtSize) {
    fmtSize = gpEventToString(&request_->_gpEvents[i], buffer, capacity);
  }

  for(; capacity && i< XPEDITE_PMC_CTRL_FIXED_EVENT_MAX; ++i, buffer += fmtSize, capacity -= fmtSize) {
    fmtSize = fixedEventToString(&request_->_fixedEvents[i], buffer, capacity);
  }

  for(; capacity && i< XPEDITE_PMC_CTRL_FIXED_EVENT_MAX; ++i, buffer += fmtSize, capacity -= fmtSize) {
    fmtSize = offcoreEventToString(&request_->_offcoreEvents[i], buffer, capacity);
  }
  return size_ - capacity;
}
