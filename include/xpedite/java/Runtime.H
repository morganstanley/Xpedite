///////////////////////////////////////////////////////////////////////////////
//
// Xpedite Java runtime to attach to JVM, get JVM environment information,
// and cache Xpedite java methods to activate probes
//
// Author: Brooke Elizabeth Cantwell, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <jni.h>
#include <xpedite/framework/Framework.H>

class Runtime {
  public:
    explicit Runtime(JavaVM* jvm_)
      : _env {}, _xpediteClass {}, _activateProbesMethod {} {
      using namespace xpedite::framework;

      if (!jvm_) {
        throw std::runtime_error {"failed to attach to JVM"};
      }
      jvm_->GetEnv(reinterpret_cast<void**>(&_env), JNI_VERSION_1_6);
      if(!_env) {
        throw std::runtime_error {"failed to get JVM environment"};
      }
      jclass cls = _env->FindClass("com/xpedite/Xpedite");
      if (_env->ExceptionCheck()) {
        _env->ExceptionClear();
        throwJavaException("java/lang/NoClassDefFoundError", "failed to locate Xpedite class");
      }
      _xpediteClass = reinterpret_cast<jclass>(_env->NewGlobalRef(cls));
      _activateProbesMethod = _env->GetStaticMethodID(_xpediteClass, "activateProbes", "([Lcom/xpedite/probes/AbstractProbe;)V");
      if (!_activateProbesMethod) {
        throwJavaException("java/lang/NoSuchMethodError", "failed to access activate probe method");
      }
      if(!xpedite::framework::initialize("xpedite-appinfo.txt", {DISABLE_REMOTE_PROFILING})) {
        throw std::runtime_error {"failed to init xpedite"};
      }
    }

    void activateProbes(jobjectArray probeArray_) {
      _env->CallStaticVoidMethod(_xpediteClass, _activateProbesMethod, probeArray_);
    }


    void throwJavaException(const char* typeName_, const char* errmsg_) {
      jclass Exception = _env->FindClass(typeName_);
      if(_env->ExceptionCheck()) {
        throw std::runtime_error {"failed to locate exception type"};
      }
      _env->ThrowNew(Exception,errmsg_);
    }

  private:
    JNIEnv* _env;
    jclass _xpediteClass;
    jmethodID _activateProbesMethod;

};
