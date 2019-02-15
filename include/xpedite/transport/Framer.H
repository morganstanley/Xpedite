////////////////////////////////////////////////////////////////////////////////////////////////
//
// Framer - provides logic to build datagrams from a stream of data.
//
// The framer aggregates data from a tcp socket, till there is enough data to build a new frame.
//
// readFrame() - each invocation provides 
//    a new data frame if available, else returns an empty frame.
//
// Author: Manikandan Dhamodharan, Morgan Stanley
//
////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Socket.H"
#include "Frame.H"
#include "Buffer.H"
#include <cstdint>

namespace xpedite { namespace transport { namespace tcp {

  class Framer
  {
    static constexpr size_t headerLen = 8;

    static constexpr size_t bufferCapacity = 8 * 1024;

    enum class CursorLocation
    {
      PDU_META, PDU_BODY, DISCONNECTED
    };

    enum class ReadStatus
    {
      ERROR, PARTIAL, COMPLETE
    };

    public:

    Framer()
      : _socket {}, _buffer {bufferCapacity}, _cursorLocation {CursorLocation::DISCONNECTED}, _frameLength {} {
    }

    explicit Framer(Socket* socket_) noexcept
      : _socket {socket_}, _buffer {bufferCapacity}, _cursorLocation {CursorLocation::PDU_META}, _frameLength {headerLen} {
    }

    Frame readFrame();

    void reset(Socket* socket_ = nullptr) {
      _socket = socket_;
      if(_socket) {
        _buffer.reset();
        _cursorLocation = CursorLocation::PDU_META;
        _frameLength = headerLen;
      } else {
        _cursorLocation = CursorLocation::DISCONNECTED;
        _frameLength = 0;
      }
    }

    private:

    ReadStatus read() noexcept;
    size_t parseFrameLen() noexcept;
    void handleDisconnect() const;

    Socket* _socket;
    util::Buffer _buffer;
    CursorLocation _cursorLocation;
    size_t _frameLength;
  };

}}}
