////////////////////////////////////////////////////////////////////////////////////////////////
//
// Xpedite frameork control api
//
// Framework initializaion creates a background thread to provide the following functionalities
//   1. Creates a non-blocking listener socket to accept tcp connections from profiler
//   2. Awaits for connections from the profiler
//   3. Timeshares between, handling of profiler connection and polling for new samples
//   4. The polling is terminated on socket disconnect
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
////////////////////////////////////////////////////////////////////////////////////////////////

#include <xpedite/transport/Listener.H>
#include <xpedite/transport/Framer.H>
#include <xpedite/framework/SamplesBuffer.H>
#include <xpedite/log/Log.H>
#include <xpedite/probes/ProbeList.H>
#include <xpedite/probes/ProbeCtl.H>
#include <xpedite/util/Tsc.H>
#include <xpedite/common/PromiseKeeper.H>
#include "Admin.H"
#include "Handler.H"
#include <thread>
#include <mutex>
#include <memory>
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <unistd.h>
#include <fstream>
#include <chrono>
#include <unistd.h>
#include <vector>
#include <atomic>
#include <sched.h>
#include <pthread.h>

namespace xpedite { namespace framework {

  class Framework;

  static std::unique_ptr<Framework> instantiateFramework(const char* appInfoFile_, const char* listenerIp_) noexcept;

  constexpr bool isListenerBlocking = false;

  class Framework
  {
    public:
      bool isActive();
      void run(std::promise<bool>& listenerInitPromise_, bool awaitProfileBegin_);
      bool isRunning() noexcept;
      bool halt() noexcept;

      ~Framework();

    private:

      Framework(const char* appInfoFile_, const char* listenerIp_);

      void handleClient(std::unique_ptr<xpedite::transport::tcp::Socket> clientSocket_, common::PromiseKeeper<bool>& promiseKeeper_) noexcept;
      std::string handleFrame(xpedite::transport::tcp::Frame frame_) noexcept;

      Framework(const Framework&) = delete;
      Framework& operator=(const Framework&) = delete;
      Framework(Framework&&) = default;
      Framework& operator=(Framework&&) = default;

      void log();

      xpedite::transport::tcp::Listener _listener;
      const char* _appInfoPath;
      std::ofstream _appInfoStream;
      Handler _handler;
      volatile std::atomic<bool> _canRun;

      friend std::unique_ptr<Framework> instantiateFramework(const char* appInfoFile_, const char* listenerIp_) noexcept;
  };

  Framework::Framework(const char* appInfoPath_, const char* listenerIp_)
    : _listener {"xpedite", isListenerBlocking, 0, listenerIp_}, _appInfoPath {appInfoPath_},
      _appInfoStream {}, _handler {}, _canRun {true} {
    try {
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
    _appInfoStream << "port: " << _listener.port() << std::endl;
     _appInfoStream<< "binary: " << xpedite::util::getExecutablePath() << std::endl;
     _appInfoStream<< "tscHz: " << tscHz << std::endl;
    log::logProbes(_appInfoStream, probes::probeList());
    _appInfoStream.close();
    XpediteLogInfo << "Xpedite app info stored at - " << _appInfoPath << XpediteLogEnd;
  }

  static std::unique_ptr<Framework> instantiateFramework(const char* appInfoFile_, const char* listenerIp_) noexcept {
    return std::unique_ptr<Framework> {new Framework {appInfoFile_, listenerIp_}};
  }

  static std::string encode(const std::string& payload) {
    std::ostringstream stream;
    stream << std::setfill('0') << std::setw(8) << payload.size() << std::setfill(' ') 
      << payload;
    return stream.str();
  }

  void Framework::run(std::promise<bool>& listenerInitPromise_, bool awaitProfileBegin_) {
    common::PromiseKeeper<bool> promiseKeeper {&listenerInitPromise_};

    if(!_handler.registerCommand("probes", admin)) {
      std::ostringstream stream;
      stream << "xpedite framework init error - Failed to register processor for probes admin";
      throw std::runtime_error {stream.str()};
    }

    if(!_listener.start()) {
      std::ostringstream stream;
      stream << "xpedite framework init error - Failed to start listener " << _listener.toString();
      throw std::runtime_error {stream.str()};
    }

    log();

    if(!awaitProfileBegin_) {
      promiseKeeper.deliver(true);
    }

    while(_canRun.load(std::memory_order_relaxed)) {
      std::unique_ptr<xpedite::transport::tcp::Socket> clientSocket = _listener.accept();
      if(clientSocket) {
        handleClient(std::move(clientSocket), promiseKeeper);
      }
      else {
        std::this_thread::sleep_for(std::chrono::duration<unsigned, std::milli> {500});
      }
    }

    if(!_canRun.load(std::memory_order_relaxed)) {
      XpediteLogCritical << "xpedite - shutting down handler/thread" << XpediteLogEnd;
      _handler.shutdown();
    }
  }

  void Framework::handleClient(std::unique_ptr<xpedite::transport::tcp::Socket> clientSocket_, common::PromiseKeeper<bool>& promiseKeeper_) noexcept {
    XpediteLogInfo << "xpedite - accepted incoming connection from " << clientSocket_->toString() << XpediteLogEnd;

    struct SessionGuard
    {
      Handler* _handler;
      explicit SessionGuard(Handler* handler_) : _handler {handler_} {_handler->beginSession();}
      ~SessionGuard() {_handler->endSession();}
    } sessionGuard {&_handler};

    try {
      xpedite::transport::tcp::Framer clientFramer {clientSocket_.get()};
      while(_canRun.load(std::memory_order_relaxed)) {
        auto frame = clientFramer.readFrame();
        if(frame) {
          std::string result = handleFrame(frame);
          std::string pdu = encode(result);
          if(clientSocket_->write(pdu.data(), pdu.size()) != static_cast<int>(pdu.size())) {
            XpediteLogCritical << "xpedite - handler error, failed to send result " 
              << result << " to client " << clientSocket_->toString() << XpediteLogEnd;
            return;
          }
        }
        if(promiseKeeper_.isPending() && _handler.isProfileActive()) {
          promiseKeeper_.deliver(true);
        }
        _handler.poll();
      }

      if(!_canRun.load(std::memory_order_relaxed)) {
        XpediteLogCritical << "xpedite - closing client connection - framework is going down." << XpediteLogEnd;
      }
    }
    catch(std::runtime_error& e_) {
      XpediteLogCritical << "xpedite - closing client connection - error " << e_.what() << XpediteLogEnd;
      return;
    }
    catch(...) {
      XpediteLogCritical << "xpedite - closing client connection - unknown error" << XpediteLogEnd;
      return;
    }
  }

  std::string Framework::handleFrame(xpedite::transport::tcp::Frame frame_) noexcept {
    XpediteLogDebug << "rx frame (" << frame_.size() << " bytes) - " 
      <<  std::string {frame_.data(), static_cast<std::size_t>(frame_.size())} << XpediteLogEnd;
    return _handler.handle(frame_.data(), frame_.size());
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

  static void initializeOnce(const char* appInfoFile_, const char* listenerIp_, bool awaitProfileBegin_, bool* rc_) noexcept {
    std::promise<bool> listenerInitPromise;
    std::future<bool> listenerInitFuture = listenerInitPromise.get_future();
    std::thread thread {
      [&listenerInitPromise, appInfoFile_, listenerIp_, awaitProfileBegin_] {
        try {
          framework = instantiateFramework(appInfoFile_, listenerIp_);
          framework->run(listenerInitPromise, awaitProfileBegin_);
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
    auto timeout = awaitProfileBegin_ ? 120 : 5;
    if(listenerInitFuture.wait_until(std::chrono::system_clock::now() + std::chrono::seconds(timeout)) != std::future_status::ready) {
      XpediteLogCritical << "xpedite - init failure - failed to start listener (timedout)" << XpediteLogEnd;
      *rc_ = false;
      return;
    }
    *rc_ = true; 
  }

  bool initialize(const char* appInfoFile_, const char* listenerIp_, bool awaitProfileBegin_) {
    bool rc {};
    std::call_once(initFlag, initializeOnce, appInfoFile_, listenerIp_, awaitProfileBegin_, &rc);
    return rc;
  }

  bool initialize(const char* appInfoFile_, bool awaitProfileBegin_) {
    return initialize(appInfoFile_, "", awaitProfileBegin_);
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

  bool isRunning() noexcept {
    if(framework) {
      return framework->isRunning();
    }
    return false;
  }

  void pinThread(unsigned core_) {
    if(isRunning()) {
      cpu_set_t cpuset;
      CPU_ZERO(&cpuset);
      CPU_SET(core_, &cpuset);
      int rc = pthread_setaffinity_np(frameworkThread.native_handle(), sizeof(cpu_set_t), &cpuset);
      if(rc != 0) {
        std::string errMsg;
        switch (rc) {
          case EFAULT:
            errMsg = "A supplied memory address was invalid";
            break;
          case EINVAL:
            errMsg = "supplied core was invalid";
            break;
          case ESRCH:
            errMsg = "thread not alive";
            break;
          default:
            errMsg = "unknown error";
            break;
        }
        std::ostringstream stream;
        stream << "xpedite - failed to pin thread [pthread_setaffinity_np error - " << rc << " | " << errMsg << "]";
        XpediteLogInfo << stream.str()<< XpediteLogEnd;
        throw std::runtime_error {stream.str()};
      }
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
