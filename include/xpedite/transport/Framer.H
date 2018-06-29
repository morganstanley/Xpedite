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

    enum class CursorLocation
    {
      PDU_META, PDU_BODY, DISCONNECTED
    };

    enum class ReadStatus
    {
      ERROR, PARTIAL, COMPLETE
    };

    public:

    Framer(Socket* socket_) noexcept
      : _socket {socket_}, _buffer {8 * 1024}, _cursorLocation {CursorLocation::PDU_META}, _frameLength {headerLen} {
    }

    Frame readFrame();

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
