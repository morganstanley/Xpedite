///////////////////////////////////////////////////////////////////////////////
//
// Java native interface functions to enable activation of Xpedite probes and
// recording in Java applications
//
//
// Author: Brooke Elizabeth Cantwell, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#include <jni.h>
#include <mutex>
#include <xpedite/framework/SamplesBuffer.H>
#include <com_xpedite_Xpedite.h>
#include <Runtime.H>

static std::mutex mutex;
static Runtime* runtime;
static xpedite::framework::SessionGuard sessionGuard;

Runtime* resolveRuntime(JavaVM* jvm) {
  std::lock_guard<std::mutex> guard{mutex};
  if (!runtime) {
    runtime = new Runtime(jvm);
  }
  return runtime;
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void*) {
  resolveRuntime(vm);
  return JNI_VERSION_1_6;
}

JNIEXPORT void JNICALL Java_com_xpedite_Xpedite_profile(JNIEnv*, jclass, jobjectArray probeArray_) {
  using namespace xpedite::probes;

  if (!probeArray_) {
    runtime->throwJavaException("java/lang/NullPointerException", "No probes to enable");
  }
  assert(runtime);

  xpedite::framework::ProfileInfo profileInfo {
    std::vector<std::string> {},
    PMUCtlRequest {
        ._cpu = 0, ._fixedEvtCount = 2, ._gpEvtCount = 0, ._offcoreEvtCount = 0,
        ._fixedEvents = {
          PMUFixedEvent {._ctrIndex = 0, ._user = 1, ._kernel = 1},
          PMUFixedEvent {._ctrIndex = 1, ._user = 1, ._kernel = 1}
        },
        ._gpEvents = {},
        ._offcoreEvents = {0, 0}
      }
  };
  sessionGuard = xpedite::framework::profile(profileInfo);

  runtime->activateProbes(probeArray_);
}

JNIEXPORT void JNICALL Java_com_xpedite_Xpedite_record(JNIEnv*, jclass, jint probeID) {
  using namespace xpedite::probes;
  auto tsc_ = RDTSC();
  void* returnSite_ = reinterpret_cast<void*>(probeID);
  xpediteRecordPerfEvents(returnSite_, tsc_);
}
