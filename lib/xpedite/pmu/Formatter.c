///////////////////////////////////////////////////////////////////////////////////////////////
//
// Logic to format pmu request and event objects to strings
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////////////////////

#include <xpedite/pmu/Formatter.h>

static int g_xpediteCanLog = 1;
int  xpediteCanLog(void)       { return g_xpediteCanLog;  }
void xpediteSupressLog(void)   { g_xpediteCanLog = 0;     }
void xpediteUnsupressLog(void) { g_xpediteCanLog = 1;     }

void logEventSet(const PMUCtlRequest* request_, const EventSet* eventSet_) {
  if(request_->_gpEvtCount) {
    if(request_->_gpEvtCount <= XPEDITE_PMC_CTRL_GP_EVENT_MAX) {
      XPEDITE_LOG("%s\n", "Core events: ");
      int i=0;
      for(; i< request_->_gpEvtCount; ++i) {
        const PMUGpEvent* e = &request_->_gpEvents[i];
        uint32_t b = eventSet_->_gpEvtSel[i];
        XPEDITE_LOG("\t\t-> eventSelect = 0x%02hhX | unitMask = 0x%02hhX | user = 0x%02hhX | kernel = 0x%02hhX |"
          " invertCMask = 0x%02hhX | counterMask = 0x%02hhX | -> PerfEvtSel%u [0x%08llX]\n", 
          e->_eventSelect, e->_unitMask, e->_user, e->_kernel, 
          e->_invertCMask, e->_counterMask, i, (unsigned long long)b
        );
      }
    } else {
      XPEDITE_LOG("\t\t-> Core events - Invalid count %hhd exceeds max count %hhd",
        request_->_gpEvtCount, XPEDITE_PMC_CTRL_GP_EVENT_MAX);
    }
  }

  if(request_->_fixedEvtCount) {
    if(request_->_fixedEvtCount <= XPEDITE_PMC_CTRL_FIXED_EVENT_MAX) {
      XPEDITE_LOG("%s\n", "Fixed events: ");
      unsigned char feMask = eventSet_->_fixedEvtGlobalCtl;
      unsigned long long fixedEvtSel = eventSet_->_fixedEvtSel;
      XPEDITE_LOG("\t\t-> Fixed events global mask = 0x%02hhX | eventSelect = 0x%08llX\n", feMask, fixedEvtSel);
    } else {
      XPEDITE_LOG("\t\t-> Fixed events - Invalid count %hhd exceeds max count %hhd",
        request_->_fixedEvtCount, XPEDITE_PMC_CTRL_FIXED_EVENT_MAX);
    }
  }

  if(request_->_offcoreEvtCount) {
    if(request_->_offcoreEvtCount < XPEDITE_PMC_CTRL_OFFCORE_EVENT_MAX) {
      XPEDITE_LOG("%s\n", "Offcore events: ");
      int i=0;
      for(; i< request_->_offcoreEvtCount; ++i) {
        XPEDITE_LOG("\t\t-> MSR_OFFCORE_RSP_%u -> %llx\n", i, (unsigned long long) request_->_offcoreEvents[i]);
      }
    } else {
      XPEDITE_LOG("\t\t-> Offcore events - Invalid count %hhd exceeds max count %hhd",
        request_->_offcoreEvtCount, XPEDITE_PMC_CTRL_OFFCORE_EVENT_MAX);
    }
  }
}

static unsigned char toBoolenChar(int v_) {
  return v_? 'y' : 'n';
}

int gpEventToString(char* buffer_, int size_, const PMUGpEvent* event_) {
  return snprintf(buffer_, size_,
    "\nCore Event [eventSelect - %2hhx, unitMask - %2hhx, user - %c, kernel - %c"
    ", invertCMask - %2hhx, counterMask - %2hhx, edgeDetect - %2hhx, anyThread - %c]",
    event_->_eventSelect, event_->_unitMask, toBoolenChar(event_->_user), toBoolenChar(event_->_kernel),
    event_->_invertCMask, event_->_counterMask, event_->_edgeDetect, toBoolenChar(event_->_anyThread)
  );
}

int fixedEventToString(char* buffer_, int size_, const PMUFixedEvent* event_) {
  return snprintf(buffer_, size_, "\nFixed Event [index - %2hhu, user - %c, kernel - %c]"
      , event_->_ctrIndex, toBoolenChar(event_->_user), toBoolenChar(event_->_kernel));
}

int offcoreEventToString(char* buffer_, int size_, const PMUOffcoreEvent* event_) {
  return snprintf(buffer_, size_, "\nOffcore Event [index - %llx]", (unsigned long long) *event_);
}

int invalidCountToString(char* buffer_, int size_, const char* eventType_, unsigned char expected_, unsigned char actual_) {
  return snprintf(
    buffer_, size_, "\n%s Events - Invalid count %hhd exceeds max count %hhd", eventType_, actual_, expected_
  );
}

void pmuRequestToString(const PMUCtlRequest* request_, char* buffer_, int size_) {

  char* buffer = buffer_;
  int capacity = size_;
  int fmtSize = 0;

  if(request_->_gpEvtCount <= XPEDITE_PMC_CTRL_GP_EVENT_MAX) {
    int i=0;
    for(; capacity > 1 && i< request_->_gpEvtCount; ++i, buffer += fmtSize, capacity -= fmtSize) {
      fmtSize = gpEventToString(buffer, capacity, &request_->_gpEvents[i]);
      if(fmtSize >= capacity) {
        return;
      }
    }
  } else if(capacity > 1) {
    fmtSize = invalidCountToString(buffer, capacity, "Core", XPEDITE_PMC_CTRL_GP_EVENT_MAX, request_->_gpEvtCount);
    if(fmtSize >= capacity) {
      return;
    }
    buffer += fmtSize;
    capacity -= fmtSize;
  }

  if(request_->_fixedEvtCount <= XPEDITE_PMC_CTRL_FIXED_EVENT_MAX) {
    int i=0;
    for(; capacity > 1 && i< request_->_fixedEvtCount; ++i, buffer += fmtSize, capacity -= fmtSize) {
      fmtSize = fixedEventToString(buffer, capacity, &request_->_fixedEvents[i]);
      if(fmtSize >= capacity) {
        return;
      }
    }
  } else if(capacity > 1) {
    fmtSize = invalidCountToString(buffer, capacity, "Fixed", XPEDITE_PMC_CTRL_FIXED_EVENT_MAX, request_->_fixedEvtCount);
    if(fmtSize >= capacity) {
      return;
    }
    buffer += fmtSize;
    capacity -= fmtSize;
  }

  if(request_->_offcoreEvtCount <= XPEDITE_PMC_CTRL_OFFCORE_EVENT_MAX) {
    int i=0;
    for(; capacity > 1 && i< request_->_offcoreEvtCount; ++i, buffer += fmtSize, capacity -= fmtSize) {
      fmtSize = offcoreEventToString(buffer, capacity, &request_->_offcoreEvents[i]);
      if(fmtSize >= capacity) {
        return;
      }
    }
  } else if(capacity > 1) {
    fmtSize = invalidCountToString(buffer, capacity, "Offcore", XPEDITE_PMC_CTRL_OFFCORE_EVENT_MAX, request_->_offcoreEvtCount);
    if(fmtSize >= capacity) {
      return;
    }
    buffer += fmtSize;
    capacity -= fmtSize;
  }
  return;
}
