///////////////////////////////////////////////////////////////////////////////////////////////
//
// Logic to format pmu request and event objects to strings
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////////////////////

#include <xpedite/pmu/Formatter.h>

void logEventSet(const PMUCtlRequest* request_, const EventSet* eventSet_) {
  int i=0;
  if(request_->_gpEvtCount) {
    XPEDITE_LOG("%s\n", "Core events: ");
    for(i=0; i< request_->_gpEvtCount; ++i) {
      const PMUGpEvent* e = &request_->_gpEvents[i];
      uint32_t b = eventSet_->_gpEvtSel[i];
      XPEDITE_LOG("\t\t-> eventSelect = 0x%02hhX | unitMask = 0x%02hhX | user = 0x%02hhX | kernel = 0x%02hhX |"
        " invertCMask = 0x%02hhX | counterMask = 0x%02hhX | -> PerfEvtSel%u [0x%08llX]\n", 
        e->_eventSelect, e->_unitMask, e->_user, e->_kernel, 
        e->_invertCMask, e->_counterMask, i, (unsigned long long)b
      );
    }
  }

  if(request_->_fixedEvtCount) {
    XPEDITE_LOG("%s\n", "Fixed events: ");
    unsigned char feMask = eventSet_->_fixedEvtGlobalCtl;
    unsigned long long fixedEvtSel = eventSet_->_fixedEvtSel;
    XPEDITE_LOG("\t\t-> Fixed events global mask = 0x%02hhX | eventSelect = 0x%08llX\n", feMask, fixedEvtSel);
  }

  if(request_->_offcoreEvtCount) {
    XPEDITE_LOG("%s\n", "Offcore events: ");
    for(i=0; i< request_->_offcoreEvtCount; ++i) {
      XPEDITE_LOG("\t\t-> MSR_OFFCORE_RSP_%u -> %llx\n", i, (unsigned long long) request_->_offcoreEvents[i]);
    }
  }
}

static unsigned char toBoolenChar(int v_) {
  return v_? 'y' : 'n';
}

int gpEventToString(const PMUGpEvent* event_, char* buffer_, int size_) {
  return snprintf(buffer_, size_,
    "\nCore Event [eventSelect - %2hhx, unitMask - %2hhx, user - %c, kernel - %c"
    ", invertCMask - %2hhx, counterMask - %2hhx, edgeDetect - %2hhx, anyThread - %c]",
    event_->_eventSelect, event_->_unitMask, toBoolenChar(event_->_user), toBoolenChar(event_->_kernel),
    event_->_invertCMask, event_->_counterMask, event_->_edgeDetect, toBoolenChar(event_->_anyThread)
  );
}

int fixedEventToString(const PMUFixedEvent* event_, char* buffer_, int size_) {
  return snprintf(buffer_, size_, "\nFixed Event [index - %2hhu, user - %c, kernel - %c]"
      , event_->_ctrIndex, toBoolenChar(event_->_user), toBoolenChar(event_->_kernel));
}

int offcoreEventToString(const PMUOffcoreEvent* event_, char* buffer_, int size_) {
  return snprintf(buffer_, size_, "\nOffcore Event [index - %llx]", (unsigned long long) *event_);
}

void pmuRequestToString(const PMUCtlRequest* request_, char* buffer_, int size_) {

  char* buffer = buffer_;
  int capacity = size_;
  int fmtSize = 0;

  int i=0;
  for(; capacity > 1 && i< request_->_gpEvtCount; ++i, buffer += fmtSize, capacity -= fmtSize) {
    fmtSize = gpEventToString(&request_->_gpEvents[i], buffer, capacity);
    if(fmtSize >= capacity) {
      return;
    }
  }

  i=0;
  for(; capacity > 1 && i< request_->_fixedEvtCount; ++i, buffer += fmtSize, capacity -= fmtSize) {
    fmtSize = fixedEventToString(&request_->_fixedEvents[i], buffer, capacity);
    if(fmtSize >= capacity) {
      return;
    }
  }

  i=0;
  for(; capacity > 1 && i< request_->_offcoreEvtCount; ++i, buffer += fmtSize, capacity -= fmtSize) {
    fmtSize = offcoreEventToString(&request_->_offcoreEvents[i], buffer, capacity);
    if(fmtSize >= capacity) {
      return;
    }
  }
  return;
}
