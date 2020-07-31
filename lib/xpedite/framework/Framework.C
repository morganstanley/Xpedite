////////////////////////////////////////////////////////////////////////////////////////////////
//
// Xpedite frameork control api
//
// Framework initializaion creates a background thread to provide the following functionalities
//   1. Creates a session manager to listen for remote tcp sessions
//   2. Awaits session establishment from local or remote profiler
//   3. Timeshares between, handling of profiler connection and polling for new samples
//   4. Clean up on session disconnect and process shutdown
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
////////////////////////////////////////////////////////////////////////////////////////////////

#include <xpedite/framework/Framework.H>
#include "session/SessionManager.H"
#include <xpedite/transport/Framer.H>
#include <xpedite/framework/SamplesBuffer.H>
#include <xpedite/log/Log.H>
#include <xpedite/probes/ProbeList.H>
#include <xpedite/util/Tsc.H>
#include <xpedite/common/PromiseKeeper.H>
#include "StorageMgr.H"
#include "request/RequestParser.H"
#include "request/ProbeRequest.H"
#include "request/ProfileRequest.H"
#include <thread>
#include <memory>
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <unistd.h>
#include <fstream>
#include <chrono>
#include <unistd.h>
#include <atomic>
#include <algorithm>

namespace xpedite { namespace framework {

  class Framework;

  static std::unique_ptr<Framework> instantiateFramework(const char*, std::vector<Option>&&, const char*, in_port_t) noexcept;

  template<typename Options>
  inline bool isEnabled(const Options& options_, Option option_) {
    return std::find(options_.begin(), options_.end(), option_) != options_.end();
  }

  class Framework
  {
    public:
      bool isActive();
      void run(std::promise<bool>& sessionInitPromise_);
      SessionGuard beginProfile(const ProfileInfo& profileInfo_);
      void endProfile();
      bool isRunning() noexcept;
      bool halt() noexcept;

      ~Framework();

    private:

      Framework(const char* appInfoFile_, std::vector<Option>&& options_, const char* listenerIp_, in_port_t port_);

      void handleClient(std::unique_ptr<xpedite::transport::tcp::Socket> clientSocket_, common::PromiseKeeper<bool>& promiseKeeper_) noexcept;
      std::string handleFrame(xpedite::transport::tcp::Frame frame_) noexcept;

      Framework(const Framework&) = delete;
      Framework& operator=(const Framework&) = delete;
      Framework(Framework&&) = default;
      Framework& operator=(Framework&&) = default;

      void log();

      const char* _appInfoPath;
      std::vector<Option> _options;
      std::ofstream _appInfoStream;
      session::SessionManager _sessionManager;
      volatile std::atomic<bool> _canRun;

      friend std::unique_ptr<Framework> instantiateFramework(const char*, std::vector<Option>&&, const char*, in_port_t) noexcept;
  };

  Framework::Framework(const char* appInfoPath_, std::vector<Option>&& options_, const char* listenerIp_, in_port_t port_)
    : _appInfoPath {appInfoPath_}, _options {std::move(options_)}, _appInfoStream {}, _sessionManager {}, _canRun {true} {
    try {
      XpediteLogInfo << "Initializing framework with options - " << toString(options_) << XpediteLogEnd;
      if(!isEnabled(_options, Option::DISABLE_REMOTE_PROFILING)) {
        _sessionManager.enableRemoteSession(listenerIp_, port_);
      }
      _appInfoStream.open(appInfoPath_, std::ios_base::out);
    }
    catch(std::ios_base::failure& e) {
      std::ostringstream stream;
      stream << "xpedite framework init error - failed to open log " << appInfoPath_ << " for writing - " << e.what();
      throw std::runtime_error {stream.str()};
    }
  }

  void Framework::log() {
    static auto tscHz = util::estimateTscHz();
    _appInfoStream << "pid: " << getpid() << std::endl;
    _appInfoStream << "port: " << _sessionManager.listenerPort() << std::endl;
     _appInfoStream<< "binary: " << xpedite::util::getExecutablePath() << std::endl;
     _appInfoStream<< "tscHz: " << tscHz << std::endl;
    log::logProbes(_appInfoStream, probes::probeList());
    _appInfoStream.close();
    XpediteLogInfo << "Xpedite app info stored at - " << _appInfoPath << XpediteLogEnd;
  }

  static std::unique_ptr<Framework> instantiateFramework(const char* appInfoFile_, std::vector<Option>&& options_, const char* listenerIp_,
      in_port_t port_) noexcept {
    return std::unique_ptr<Framework> {new Framework {appInfoFile_, std::move(options_), listenerIp_, port_}};
  }

  void Framework::run(std::promise<bool>& sessionInitPromise_) {
    common::PromiseKeeper<bool> promiseKeeper {&sessionInitPromise_};

    _sessionManager.start();

    log();

    if(!isEnabled(_options, Option::AWAIT_PROFILE_BEGIN)) {
      promiseKeeper.deliver(true);
    }

    while(_canRun.load(std::memory_order_relaxed)) {
      _sessionManager.poll();
      if(promiseKeeper.isPending() && _sessionManager.isProfileActive()) {
        promiseKeeper.deliver(true);
      }
      std::this_thread::sleep_for(_sessionManager.pollInterval());
    }

    if(!_canRun.load(std::memory_order_relaxed)) {
      XpediteLogCritical << "xpedite - shutting down handler/thread" << XpediteLogEnd;
      _sessionManager.shutdown();
    }
  }

  std::vector<probes::Probe*> findProbesByName(const char* name_) {
    return probes::probeList().findByName(name_);
  }

  std::vector<probes::Probe*> findProbesByLocation(const char* file_, uint32_t line_) noexcept {
    return probes::probeList().findByLocation(file_, line_);
  }

  probes::Probe* findProbeByReturnSite(const void* returnSite_) noexcept {
    return probes::probeList().findByReturnSite(returnSite_);
  }

  SessionGuard Framework::beginProfile(const ProfileInfo& profileInfo_) {
    using namespace xpedite::framework::request;
    SessionGuard guard {true};
    if(eventCount(&profileInfo_.pmuRequest())) {
      PerfEventsActivationRequest perfEventsRequest {profileInfo_.pmuRequest()};
      if(!_sessionManager.execute(&perfEventsRequest)) {
        std::ostringstream stream;
        stream << "xpedite failed to enable perf events - " << perfEventsRequest.response().errors();
        XpediteLogCritical <<  stream.str() << XpediteLogEnd;
        return SessionGuard {stream.str()};
      }
    }

    ProbeActivationRequest probeActivationRequest {profileInfo_.probes()};
    if(!_sessionManager.execute(&probeActivationRequest)) {
      std::ostringstream stream;
      stream << "xpedite failed to enable probes - " << probeActivationRequest.response().errors();
      XpediteLogCritical <<  stream.str() << XpediteLogEnd;
      return SessionGuard {stream.str()};
    }

    ProfileActivationRequest profileActivationRequest {
      StorageMgr::buildSamplesFileTemplate(), MilliSeconds {1}, profileInfo_.samplesDataCapacity()
    };
    profileActivationRequest.overrideRecorder(profileInfo_.recorder(), profileInfo_.dataProbeRecorder());
    if(!_sessionManager.execute(&profileActivationRequest)) {
      std::ostringstream stream;
      stream << "xpedite failed to activate profile - " << profileActivationRequest.response().errors();
      XpediteLogCritical <<  stream.str() << XpediteLogEnd;
      return SessionGuard {stream.str()};
    }
    return guard;
  }

  void Framework::endProfile() {
    request::ProfileDeactivationRequest profileDeactivationRequest {};
    if(!_sessionManager.execute(&profileDeactivationRequest)) {
      XpediteLogCritical << "xpedite - failed to deactivate profile - " << profileDeactivationRequest.response().errors()
        << XpediteLogEnd;
    }
  }

  Framework::~Framework() {
    if(isRunning()) {
      XpediteLogInfo << "xpedite - framework awaiting thread shutdown, before destruction" << XpediteLogEnd;
      halt();
    }
  }

  bool Framework::isRunning() noexcept {
    return _canRun.load(std::memory_order_relaxed);
  }

  static std::once_flag initFlag;

  static std::thread frameworkThread {};

  static std::unique_ptr<Framework> framework {};

  bool Framework::halt() noexcept {
    auto isRunning = _canRun.exchange(false, std::memory_order_relaxed);
    if(isRunning) {
      XpediteLogInfo << "xpedite - framework awaiting thread shutdown" << XpediteLogEnd;
      frameworkThread.join();
    }
    return isRunning;
  }

  bool initializeThread() {
    static __thread bool threadInitFlag {};
    if(!threadInitFlag) {
      auto tid = util::gettid();
      XpediteLogInfo << "xpedite - initializing framework for thread - " << tid << XpediteLogEnd;
      SamplesBuffer::expand();
      threadInitFlag = true;
      return true;
    }
    return false;
  }

  static void initializeOnce(const char* appInfoFile_, Options options_, const char* listenerIp_, in_port_t port_, bool* rc_) noexcept {
    std::promise<bool> sessionInitPromise;
    std::future<bool> listenerInitFuture = sessionInitPromise.get_future();
    static std::vector<Option> options {options_};
    std::thread thread {
      [&sessionInitPromise, appInfoFile_, listenerIp_, port_] {
        try {
          framework = instantiateFramework(appInfoFile_, std::move(options), listenerIp_, port_);
          framework->run(sessionInitPromise);
        }
        catch(const std::exception& e) {
          XpediteLogCritical << "xpedite - init failed - " << e.what() << XpediteLogEnd;
        }
        catch(...) {
          XpediteLogCritical << "xpedite - init failed - unknown failure" << XpediteLogEnd;
        }
      }
    };
    frameworkThread = std::move(thread);

    // longer timeout, if the framework is awaiting perf client to begin profile
    auto timeout = isEnabled(options_, Option::AWAIT_PROFILE_BEGIN) ? 120 : 5;
    if(listenerInitFuture.wait_until(std::chrono::system_clock::now() + std::chrono::seconds(timeout)) != std::future_status::ready) {
      XpediteLogCritical << "xpedite - init failure - failed to start listener (timedout)" << XpediteLogEnd;
      *rc_ = false;
      return;
    }
    *rc_ = true; 
  }

  bool initialize(const char* appInfoFile_, const char* listenerIp_, int port_, Options options_) {
    initializeThread();
    bool rc {isRunning()};
    std::call_once(initFlag, initializeOnce, appInfoFile_, options_, listenerIp_, static_cast<in_port_t>(port_), &rc);
    return rc;
  }

  bool initialize(const char* appInfoFile_, Options options_) {
    return initialize(appInfoFile_, "", {}, options_);
  }

  SessionGuard profile(const ProfileInfo& profileInfo_) {
    if(framework) {
      return framework->beginProfile(profileInfo_);
    }
    return {};
  }

  SessionGuard::~SessionGuard() {
    if(_isAlive && framework) {
      XpediteLogInfo << "Live session guard being destroyed - end active profile session" << XpediteLogEnd;
      _isAlive = {};
      framework->endProfile();
    }
  }

  bool isRunning() noexcept {
    if(framework) {
      return framework->isRunning();
    }
    return false;
  }

  void pinThread(unsigned core_) {
    if(isRunning()) {
      util::pinThread(frameworkThread.native_handle(), core_);
      return;
    }
    throw std::runtime_error {"xpedite framework not initialized - no thread to pin"};
  }

  bool halt() noexcept {
    if(framework) {
      return framework->halt();
    }
    return false;
  }

}}
