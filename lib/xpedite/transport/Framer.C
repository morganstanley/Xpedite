///////////////////////////////////////////////////////////////////////////////
//
// Framer - provides logic to build datagrams from a stream of data.
//
// The framer expects the stream to be composed of length prefixed datagrams.
//
// readFrame() attemts to read 8 bytes first to extract the length of the datagram.
// Once a length is determined, the framer accumulates bytes, till it has
// enough data to completely construct the frame.
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
///////////////////////////////////////////////////////////////////////////////

#include <xpedite/transport/Framer.H>
#include <xpedite/log/Log.H>
#include <xpedite/util/Errno.H>
#include <memory>

namespace xpedite { namespace transport { namespace tcp {

  inline size_t Framer::parseFrameLen() noexcept {
    auto src = _buffer.getReadBuffer();
    _buffer.advanceReadUnsafe(headerLen);
    return (src[0] & 0xf) * 10000000
         + (src[1] & 0xf) * 1000000
         + (src[2] & 0xf) * 100000
         + (src[3] & 0xf) * 10000
         + (src[4] & 0xf) * 1000
         + (src[5] & 0xf) * 100
         + (src[6] & 0xf) * 10
         + (src[7] & 0xf);
  }

  inline Framer::ReadStatus Framer::read() noexcept {
    if(_buffer.size() < _frameLength) {
      _buffer.ensureRoom(_frameLength - _buffer.size());
      int rc = _buffer.read(_socket->fd());
      if(XPEDITE_UNLIKELY(rc < 0)) {
        XpediteLogError << "TCP framer - error reading socket (rc=" << rc << ")" << XpediteLogEnd;
        return ReadStatus::ERROR;
      }
    }
    return _buffer.size() < _frameLength ? ReadStatus::PARTIAL : ReadStatus::COMPLETE;
  }

  Frame Framer::readFrame() {
    if(_cursorLocation != CursorLocation::DISCONNECTED) {
      switch(read()) {
        case ReadStatus::COMPLETE:
          switch(_cursorLocation) {
            case CursorLocation::PDU_META:
              _frameLength = parseFrameLen();
              _cursorLocation = CursorLocation::PDU_BODY;
              if(_buffer.size() < _frameLength) {
                return {};
              }
              [[gnu::fallthrough]];
            case CursorLocation::PDU_BODY: {
                Frame frame {_buffer.getReadBuffer(), _frameLength};
                _buffer.advanceReadUnsafe(_frameLength);
                _cursorLocation = CursorLocation::PDU_META;
                _frameLength = headerLen;
                return frame;
              }
            case CursorLocation::DISCONNECTED:
              throw std::runtime_error {"Framer invariant violation - detected premature disconnect"};
          }
          break;
        case ReadStatus::ERROR:
          _cursorLocation = CursorLocation::DISCONNECTED;
           handleDisconnect();
           [[gnu::fallthrough]];
        case ReadStatus::PARTIAL:
          return {};
      }
    }
    return {};
  }

  void Framer::handleDisconnect() const {
    xpedite::util::Errno e;
    std::ostringstream stream;
    stream << "socket " << _socket->toString() << " disconnected - " << e.asString();
    throw std::runtime_error {stream.str()};
  }

}}}
