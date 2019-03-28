///////////////////////////////////////////////////////////////////////////////////////////////
//
// Xpedite target to demo statistics collection using custom sample recorders
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////////////////////

#include <xpedite/framework/Framework.H>
#include <xpedite/framework/Probes.H>
#include "../util/Args.H"
#include <stdexcept>
#include <limits>

struct StatsBuilder
{
  uint64_t _beginTsc;
  uint64_t _count;
  uint64_t _sum;
  uint64_t _min;
  uint64_t _max;
  uint64_t _mean;

  StatsBuilder()
    : _beginTsc {}, _count {}, _sum {}, _min {std::numeric_limits<uint64_t>::max()}, _max {}, _mean {} {
  }

  public:

  void recordBegin(uint64_t tsc_) {
    _beginTsc = tsc_;
  }

  void recordEnd(uint64_t tsc_) {
    auto duration = tsc_ - _beginTsc;
    _sum += duration;
    ++_count;
    if(_min > duration) {
      _min = duration;
    }
    if(_max < duration) {
      _max = duration;
    }
    _mean = _sum / _count;
  }
};

static int sampleCount;
static StatsBuilder statsBuilder;

void XPEDITE_CALLBACK recordSample(const void* returnSite_, uint64_t tsc_) {
  auto* probe = xpedite::framework::findProbeByReturnSite(returnSite_);
  std::cout << "recording sample for probe " << probe->name() << " | id - " << ++sampleCount << std::endl;

  if(probe->matchName("TxnBegin")) {
    statsBuilder.recordBegin(tsc_);
  } else if(probe->matchName("TxnEnd")) {
    statsBuilder.recordEnd(tsc_);
  }
}

void XPEDITE_CALLBACK recordSampleWithData(const void* returnSite_, uint64_t, __uint128_t data_) {
  union
  {
    uint64_t _quadWords[sizeof(__uint128_t)/sizeof(uint64_t)];
    __uint128_t _doubleQuad;
  } data;
  data._doubleQuad = data_;

  auto* probe = xpedite::framework::findProbeByReturnSite(returnSite_);
  std::cout << "recording data sample for probe " << probe->name() << " | id - " << ++sampleCount
    << ", data - [" << data._quadWords[0] << " | " << data._quadWords[1] << "]" << std::endl;
}

int main(int argc_, char** argv_) {
  auto args = parseArgs(argc_, argv_);

  using namespace xpedite::framework;
  if(!xpedite::framework::initialize("xpedite-appinfo.txt", {DISABLE_REMOTE_PROFILING})) { 
    throw std::runtime_error {"failed to init xpedite"}; 
  }

  xpedite::framework::ProfileInfo profileInfo {
    std::vector<std::string> {"TxnBegin", "TxnData", "TxnEnd"},
    PMUCtlRequest {}
  };
  profileInfo.overrideRecorder(recordSample, recordSampleWithData);
  auto guard = xpedite::framework::profile(profileInfo);

  for(int i=0; i<args.txnCount; ++i)
  {
    XPEDITE_TXN_SCOPE(Txn);
    XPEDITE_DATA_PROBE(TxnData, static_cast<unsigned>(i));
  }
  std::cout << "Statistics [min - " << statsBuilder._min
            << " | max - " << statsBuilder._max
            << " | mean - " << statsBuilder._mean
            << "] cycles" << std::endl;
  return 0;
}
