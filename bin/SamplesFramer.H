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

#include <xpedite/transport/Buffer.H>
#include <xpedite/transport/Frame.H>
#include <cstdint>
#include <xpedite/framework/Persister.H>

namespace xpedite { namespace framework { 

  class Parser {
    public:
    
    virtual size_t hdrSize() = 0;
    virtual size_t parseHeader( char *) = 0;
  };

  class FileHdrParser : public Parser{
    public:

    const xpedite::framework::FileHeader* _fileHeader;

    size_t hdrSize(){ return sizeof(xpedite::framework::FileHeader); }

    size_t parseHeader(char* ptr){
      _fileHeader = reinterpret_cast<const xpedite::framework::FileHeader*>(ptr);
      if(!_fileHeader->isValid()){ throw std::runtime_error {"invalid file header"}; }
      return _fileHeader->getSize() + sizeof(xpedite::framework::FileHeader); 
    }

    FileHdrParser & operator=(const FileHdrParser & FileParser){ return *this; }
  };

  class SegHdrParser : public Parser{

    public:

    const xpedite::framework::SegmentHeader* _segmentHeader;

    size_t hdrSize(){ return sizeof(xpedite::framework::SegmentHeader); }

    size_t parseHeader(char* ptr){
      _segmentHeader = reinterpret_cast<const xpedite::framework::SegmentHeader*>(ptr);
      if(!_segmentHeader->isValid()){ throw std::runtime_error {"invalid segment header1"}; }
      return _segmentHeader->size() + sizeof(xpedite::framework::SegmentHeader); 
    }

    SegHdrParser & operator=(const SegHdrParser & SegParser){ return *this; }
  };

  class Framer
  {
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
      : _fd {}, _parser {}, _buffer {bufferCapacity}, _cursorLocation {CursorLocation::DISCONNECTED}, _frameLength {}, _mode {BATCH} {
    }

    explicit Framer(int fd_, Parser* parser_, enum MODE mode_) noexcept
      : _fd {fd_}, _parser {parser_}, _buffer {bufferCapacity, mode_}, _cursorLocation {CursorLocation::PDU_META}, _frameLength {parser_->hdrSize()}, _mode {mode_} {
    }

    void resetParser(Parser* parser_) {
      _parser = parser_;
      _cursorLocation = CursorLocation::PDU_META;
      _frameLength = parser_->hdrSize();
    }

    xpedite::transport::tcp::Frame readFrame();

    void reset(int fd_) {
      _fd = fd_;
      if(_fd >= 0) {
        _buffer.reset();
        _cursorLocation = CursorLocation::PDU_META;
        _frameLength = _parser->hdrSize();
      } else {
        _cursorLocation = CursorLocation::DISCONNECTED;
        _frameLength = 0;
      }
    }

    private:

    ReadStatus read() noexcept;
    size_t parseFrameLen() noexcept;

    int _fd;
    Parser* _parser;
    transport::util::Buffer _buffer;
    CursorLocation _cursorLocation;
    size_t _frameLength;
    xpedite::transport::tcp::Frame header;
    enum MODE _mode;
  };

}}
