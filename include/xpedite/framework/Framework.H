///////////////////////////////////////////////////////////////////////////////////////////////
//
// Xpedite frameork control api
//
// To enable profiling, the app needs to invoke on the initialize() methods.
// As part of initialization, Xpedite will listen for incoming tcp connections from profilers. 
// The application can be profiled by, only one profiler at a time.
//
// Initialization also spawn a background thread to provide the following services
//  1. Accept new tcp connections from profiler
//  2. Process commands to query/update probe and pmc state
//  3. Collection of counted data, from probes in application threads, using a wait free buffer.
//
// The framework can be shutdown by calling halt()
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

namespace xpedite { namespace framework {

  bool initialize(const char* appInfoFile_, bool awaitProfileBegin_ = {});

  bool initialize(const char* appInfoFile_, const char* listenerIp_, bool awaitProfileBegin_ = {});

  bool initializeThread();

  bool isRunning() noexcept;

  void pinThread(unsigned core_);

  bool halt() noexcept;

}}
