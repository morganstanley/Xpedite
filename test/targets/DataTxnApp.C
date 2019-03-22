///////////////////////////////////////////////////////////////////////////////////////////////
//
// Xpedite target app to exercise txn carrying different units of data
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////////////////////

#include <xpedite/framework/Framework.H>
#include <xpedite/framework/Probes.H>
#include "../util/Args.H"
#include <stdexcept>

int main(int argc_, char** argv_) {

  auto args = parseArgs(argc_, argv_);

  using namespace xpedite::framework;
  if(!xpedite::framework::initialize("xpedite-appinfo.txt", {AWAIT_PROFILE_BEGIN})) { 
    throw std::runtime_error {"failed to init xpedite"}; 
  }

  for(int i=0; i < args.txnCount; ++i) {
    XPEDITE_TXN_SCOPE(DataTxn);

    XPEDITE_DATA_PROBE(ProbeDataPayload, xpedite::framework::ProbeData {static_cast<uint64_t>(i)});
    std::cout << "Double Quad payload" << std::endl;

    uint8_t byte = i;
    XPEDITE_DATA_PROBE(BytePayload,
      byte, byte, byte, byte, byte, byte, byte, byte,
      byte, byte, byte, byte, byte, byte, byte, byte
    );
    std::cout << "Byte payload" << std::endl;

    uint16_t word = i;
    XPEDITE_DATA_PROBE(WordPayload, word, word, word, word, word, word, word, word);
    std::cout << "Word payload" << std::endl;

    uint32_t dword = i;
    XPEDITE_DATA_PROBE(DoubleWordPayload, dword, dword, dword, dword);
    std::cout << "Double word payload" << std::endl;

    uint64_t qword = i;
    XPEDITE_DATA_PROBE(QuadWordPayload, qword, qword);
    std::cout << "Quad word payload" << std::endl;

    XPEDITE_DATA_PROBE(DoubleQuadPayload, static_cast<__uint128_t>(i));
    std::cout << "Double Quad payload" << std::endl;
  }

  for(int i=0; i < args.txnCount; ++i) {
    XPEDITE_TXN_SCOPE(DataScopedTxn);

    xpedite::framework::ProbeData probeData {static_cast<uint64_t>(2*i)};
    XPEDITE_DATA_PROBE_SCOPE(ProbeDataPayload, probeData);
    std::cout << "Double Quad payload" << std::endl;

    uint8_t byte = i;
    XPEDITE_DATA_PROBE_SCOPE(BytePayload,
      byte, byte, byte, byte, byte, byte, byte, byte,
      byte, byte, byte, byte, byte, byte, byte, byte
    );
    std::cout << "Byte payload" << std::endl;

    uint16_t word = i;
    XPEDITE_DATA_PROBE_SCOPE(WordPayload, word, word, word, word, word, word, word, word);
    std::cout << "Word payload" << std::endl;

    uint32_t dword = i;
    XPEDITE_DATA_PROBE_SCOPE(DoubleWordPayload, dword, dword, dword, dword);
    std::cout << "Double word payload" << std::endl;

    uint64_t qword = i;
    XPEDITE_DATA_PROBE_SCOPE(QuadWordPayload, qword, qword);
    std::cout << "Quad word payload" << std::endl;

    XPEDITE_DATA_PROBE_SCOPE(DoubleQuadPayload, static_cast<__uint128_t>(i));
    std::cout << "Double Quad payload" << std::endl;
    probeData.set<uint64_t, 0>(static_cast<uint64_t>(2*i + 1));
  }
  return 0;
}
