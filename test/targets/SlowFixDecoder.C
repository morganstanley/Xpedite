///////////////////////////////////////////////////////////////////////////////
//
// A stand alone program, demonstrating instrumentation and profiling with Xpedite
//
// The program accepts the following command line arguments
//  -m Creates multiple threads for profiling
//  -t Transaction count, number of memory accesses done by the program
//  -c Pin threads to CPU
//
///////////////////////////////////////////////////////////////////////////////

#include <xpedite/framework/Framework.H>
#include <xpedite/framework/Probes.H>
#include <stdexcept>
#include <cstdlib>
#include <sys/mman.h>
#include <unistd.h>
#include "../util/Args.H"

int cpu {0};

class SlowFixDecoder {
  char clOrderId[32];
  char symbol[8];
  char host[64];
  double price;
  int qty;
  int timeInForce;
  char beginMsg[8];
  int bodyLength;
  char messageType[8];
  int messageSeqNum;
  char sendingTime[32];
  char account[32];
  int handlerInst;
  int side;
  int orderType;
  char senderCompId[32];

  void parseString(char *dest, const char *src, int tag) {
    char buf[32];
    uint64_t taglen = sprintf(buf, "%d=", tag);
    const char *pos = strstr(src, buf);
    uint64_t size = strchr(pos, '\001') - pos - taglen;

    memcpy(dest, pos + taglen, size);
    dest[size] = '\0';
  }
  
  double parseDouble(const char *src, int tag) {
    char buf[32];
    char dest[64];
    uint64_t taglen = sprintf(buf, "%d=", tag);
    const char *pos = strstr(src, buf);
    uint64_t size = strchr(pos, '\001') - pos - taglen;

    memcpy(dest, pos + taglen, size);
    dest[size] = '\0';
    return std::stod(dest);
  }

  int parseInt(const char *src, int tag) {
    char buf[32];
    char dest[64];
    uint64_t taglen = sprintf(buf, "%d=", tag);
    const char *pos = strstr(src, buf);
    uint64_t size = strchr(pos, '\001') - pos - taglen;

    memcpy(dest, pos + taglen, size);
    dest[size] = '\0';
    return std::stoi(dest);
  }

public:
  void parse(const std::string &msg) {
    const char *data = msg.data();

    XPEDITE_PROBE(ParseBeginMsg);
    parseString(this->beginMsg, data, 8);
    XPEDITE_PROBE(ParseBodyLength);
    this->bodyLength = parseInt(data, 9);
    XPEDITE_PROBE(ParseMessageType);
    parseString(this->messageType, data, 35);
    XPEDITE_PROBE(ParseMessageSeqNum);
    this->messageSeqNum = parseInt(data, 34);
    XPEDITE_PROBE(ParseSendingTime);
    parseString(this->sendingTime, data, 52);
    XPEDITE_PROBE(ParseAccount);
    parseString(this->account, data, 1);
    XPEDITE_PROBE(ParseClOrderId);
    parseString(this->clOrderId, data, 11);
    XPEDITE_PROBE(ParsePrice);
    this->price = parseDouble(data, 44);
    XPEDITE_PROBE(ParseHandlerInst);
    this->handlerInst = parseInt(data, 21);
    XPEDITE_PROBE(ParseSide);
    this->side = parseInt(data, 54);
    XPEDITE_PROBE(ParseOrderType);
    this->orderType = parseInt(data, 40);
    XPEDITE_PROBE(ParseTimeInForce);
    this->timeInForce = parseInt(data, 59);
    XPEDITE_PROBE(ParseSymbol);
    parseString(this->symbol, data, 55);
    XPEDITE_PROBE(ParseQty);
    this->qty = parseInt(data, 38);
    XPEDITE_PROBE(ParseSenderCompId);
    parseString(this->senderCompId, data, 49);
    XPEDITE_PROBE(ParseHost);
    parseString(this->host, data, 56);
  }

};

static std::string fixMsgs[] {
"8=FIX.4.2\0019=299\00135=D\00134=0\00152=20160318-19:21:04.857\0011=ABCDEFG\00111=0123456789012345678912345678\00144=15.00\00121=1\00154=1\00140=2\00159=3\00155=SCSS\00138=10000\00149=fixparser\00156=example.host@xyz.com\00110=051\001",
"8=FIX.4.2\0019=298\00135=D\00134=0\00152=20160318-19:21:04.857\0011=ABCDEFG\00111=0123456789012345678912345678\00144=15.00\00121=1\00154=1\00140=2\00159=3\00155=NGG\00138=10000\00149=fixparser\00156=example.host@xyz.com\00110=082\001",
"8=FIX.4.2\0019=300\00135=D\00134=0\00152=20160318-19:21:04.857\0011=ABCDEFG\00111=0123456789012345678912345678\00144=15.00\00121=1\00154=1\00140=2\00159=3\00155=LBTYK\00138=10000\00149=fixparser\00156=example.host@xyz.com\00110=058\001",
"8=FIX.4.2\0019=299\00135=D\00134=0\00152=20160318-19:21:04.857\0011=ABCDEFG\00111=0123456789012345678912345678\00144=15.00\00121=1\00154=1\00140=2\00159=3\00155=PBCT\00138=10000\00149=fixparser\00156=example.host@xyz.com\00110=198\001",
"8=FIX.4.2\0019=298\00135=D\00134=0\00152=20160318-19:21:04.857\0011=ABCDEFG\00111=0123456789012345678912345678\00144=15.00\00121=1\00154=1\00140=2\00159=3\00155=SKX\00138=10000\00149=fixparser\00156=example.host@xyz.com\00110=034\001",
"8=FIX.4.2\0019=298\00135=D\00134=0\00152=20160318-19:21:04.857\0011=ABCDEFG\00111=0123456789012345678912345678\00144=15.00\00121=1\00154=1\00140=2\00159=3\00155=TMF\00138=10000\00149=fixparser\00156=example.host@xyz.com\00110=227\001",
"8=FIX.4.2\0019=298\00135=D\00134=0\00152=20160318-19:21:04.857\0011=ABCDEFG\00111=0123456789012345678912345678\00144=15.00\00121=1\00154=1\00140=2\00159=3\00155=LOW\00138=10000\00149=fixparser\00156=example.host@xyz.com\00110=036\001",
"8=FIX.4.2\0019=299\00135=D\00134=0\00152=20160318-19:21:04.857\0011=ABCDEFG\00111=0123456789012345678912345678\00144=15.00\00121=1\00154=1\00140=2\00159=3\00155=SCHN\00138=10000\00149=fixparser\00156=example.host@xyz.com\00110=099\001",
"8=FIX.4.2\0019=299\00135=D\00134=0\00152=20160318-19:21:04.857\0011=ABCDEFG\00111=0123456789012345678912345678\00144=15.00\00121=1\00154=1\00140=2\00159=3\00155=UVXY\00138=10000\00149=fixparser\00156=example.host@xyz.com\00110=045\001",
"8=FIX.4.2\0019=298\00135=D\00134=0\00152=20160318-19:21:04.857\0011=ABCDEFG\00111=0123456789012345678912345678\00144=15.00\00121=1\00154=1\00140=2\00159=3\00155=RIG\00138=10000\00149=fixparser\00156=example.host@xyz.com\00110=116\001",
};


inline void parseFix(uint32_t txnCount) {
  int messages = sizeof(fixMsgs) / sizeof(fixMsgs[0]);
  SlowFixDecoder fix[messages] {};
  for(uint32_t i=0; i<txnCount; ++i) {
    XPEDITE_TXN_SCOPE(ParseFix);
    std::string &str = fixMsgs[i % messages];
    fix[i % messages].parse(str);
  }
}

int main(int argc_, char** argv_) {
  auto args = parseArgs(argc_, argv_);
  int txnCount = args.txnCount;

  using namespace xpedite::framework;
  if(!xpedite::framework::initialize("xpedite-appinfo.txt", {AWAIT_PROFILE_BEGIN})) {
    throw std::runtime_error {"failed to init xpedite"};
  }

  parseFix(txnCount);

  return 0;
}
