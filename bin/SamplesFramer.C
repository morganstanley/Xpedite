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

#include "SamplesFramer.H"
#include <xpedite/log/Log.H>
#include <xpedite/util/Errno.H>
#include <memory>

namespace xpedite { namespace framework { 

  inline Framer::ReadStatus Framer::read() noexcept {
    if(_buffer.size() < _frameLength) {
      xpedite::framework::SegmentHeader* segmentHeader = reinterpret_cast<xpedite::framework::SegmentHeader*>(_buffer.getReadBuffer());
      _buffer.ensureRoom(_frameLength - _buffer.size());
      segmentHeader = reinterpret_cast<xpedite::framework::SegmentHeader*>(_buffer.getReadBuffer());
      _buffer.updateReadSize(_frameLength - _buffer.size());
      int rc = _buffer.read(_fd);
      if(XPEDITE_UNLIKELY(rc < 0)) {
        if(_mode == TRANSPORT)
          XpediteLogDebug << "Likely reaching EOF (rc=" << rc << ")" << XpediteLogEnd;
        return ReadStatus::ERROR;
      }
    }
    return _buffer.size() < _frameLength ? ReadStatus::PARTIAL : ReadStatus::COMPLETE;
  }



  xpedite::transport::tcp::Frame Framer::readFrame() {
    if(_cursorLocation != CursorLocation::DISCONNECTED) {
      switch(read()) {
        case ReadStatus::COMPLETE: 
          switch(_cursorLocation) {
            case CursorLocation::PDU_META:
              _frameLength = _parser->parseHeader(_buffer.getReadBuffer());
              _cursorLocation = CursorLocation::PDU_BODY;
              if(_buffer.size() < _frameLength + _parser->hdrSize()) {
                return {};
              }
              [[gnu::fallthrough]];
            case CursorLocation::PDU_BODY: {
              xpedite::transport::tcp::Frame frame {_buffer.getReadBuffer(), _frameLength, 0};
              _buffer.advanceReadUnsafe(_frameLength);
              _cursorLocation = CursorLocation::PDU_META;
              _frameLength = _parser->hdrSize();
              return frame;
              }
            case CursorLocation::DISCONNECTED:
              throw std::runtime_error {"Framer invariant violation - detected premature disconnect"};
          }
          break;
        case ReadStatus::ERROR:{
          xpedite::transport::tcp::Frame EOFFlag {NULL, 0, 1};
          return EOFFlag;
        }
           [[gnu::fallthrough]];
        case ReadStatus::PARTIAL:
          std::cout<<"partial"<<std::endl;
          return {};
      }
    }
    return {};
  }

}}
