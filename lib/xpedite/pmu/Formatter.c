///////////////////////////////////////////////////////////////////////////////////////////////
//
// Logic to format pmu request and event objects to strings
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////////////////////

#include <xpedite/pmu/Formatter.h>

void logRequest(unsigned ctrIndex_, const PMUGpEvent* e_, uint32_t b_) {
  XPEDITE_LOG("eventSelect = 0x%02hhX | unitMask = 0x%02hhX | user = 0x%02hhX | kernel = 0x%02hhX |"
    " invertCMask = 0x%02hhX | counterMask = 0x%02hhX | -> PerfEvtSel%u [0x%08llX]\n", 
    e_->_eventSelect, e_->_unitMask, e_->_user, e_->_kernel, 
    e_->_invertCMask, e_->_counterMask, ctrIndex_, (unsigned long long)b_
  );
}

void logOffcoreRequest(unsigned ctrIndex_, uint64_t e_) {
  XPEDITE_LOG("setting MSR_OFFCORE_RSP_%u -> %llx\n", ctrIndex_, (unsigned long long) e_);
}

static unsigned char toBoolenChar(int v_) {
  return v_? 'y' : 'n';
}

int gpEventToString(const PMUGpEvent* event_, char* buffer_, int size_) {
  return snprintf(buffer_, size_,
    "\nGpEvent {eventSelect - %2hhx, unitMask - %2hhx, user - %c, kernel - %c"
    ", invertCMask - %2hhx, counterMask - %2hhx, edgeDetect - %2hhx, anyThread - %c}",
    event_->_eventSelect, event_->_unitMask, toBoolenChar(event_->_user), toBoolenChar(event_->_kernel),
    event_->_invertCMask, event_->_counterMask, event_->_edgeDetect, toBoolenChar(event_->_anyThread)
  );
}

int fixedEventToString(const PMUFixedEvent* event_, char* buffer_, int size_) {
  return snprintf(buffer_, size_, "\nFixedEvent {index - %2hhu, user - %c, kernel - %c"
      , event_->_ctrIndex, toBoolenChar(event_->_user), toBoolenChar(event_->_kernel));
}

int offcoreEventToString(const PMUOffcoreEvent* event_, char* buffer_, int size_) {
  return snprintf(buffer_, size_, "\nOffcoreEvent {index - %llx", (unsigned long long) *event_);
}

int pmcrqToString(const PMUCtlRequest* request_, char* buffer_, int size_) {

  char* buffer = buffer_;
  int capacity = size_;
  int fmtSize = 0;

  int i=0;
  for(; capacity > 1 && i< XPEDITE_PMC_CTRL_GP_EVENT_MAX; ++i, buffer += fmtSize, capacity -= fmtSize) {
    fmtSize = gpEventToString(&request_->_gpEvents[i], buffer, capacity);
    if(fmtSize < 0) {
      return 0;
    }
    fmtSize = (fmtSize < capacity ? fmtSize : capacity - 1);
  }

  i=0;
  for(; capacity > 1 && i< XPEDITE_PMC_CTRL_FIXED_EVENT_MAX; ++i, buffer += fmtSize, capacity -= fmtSize) {
    fmtSize = fixedEventToString(&request_->_fixedEvents[i], buffer, capacity);
    if(fmtSize < 0) {
      return 0;
    }
    fmtSize = (fmtSize < capacity ? fmtSize : capacity - 1);
  }

  i=0;
  for(; capacity > 1 && i< XPEDITE_PMC_CTRL_OFFCORE_EVENT_MAX; ++i, buffer += fmtSize, capacity -= fmtSize) {
    fmtSize = offcoreEventToString(&request_->_offcoreEvents[i], buffer, capacity);
    if(fmtSize < 0) {
      return 0;
    }
    fmtSize = (fmtSize < capacity ? fmtSize : capacity - 1);
  }
  return size_ - capacity;
}
