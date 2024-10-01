#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ) {}

bool Writer::is_closed() const
{
  return closed_;
}

void Writer::push( string data )
{
  // Your code here.
  //choose between the lowest number of bytes
  auto write_count = min( 
  available_capacity(), 
  data.size()
  );
  //add total bytes processed.
  bytes_pushed_ += write_count;
  //add 
  buffer_ += data.substr( 0, write_count  );
  (void)data;
  return;
}

void Writer::close()
{
  // Your code here.
  //close Bytestream.
  closed_ = true;
  return _closed;
}

uint64_t Writer::available_capacity() const
{
  // Your code here.
  //total capacity in the buffer - current amount of bytes in buffer.
  return capacity_ - buffer_.size();
}

uint64_t Writer::bytes_pushed() const
{
  // Your code here.
  //total processed sent into stream
  return _bytes_pushed;
}

bool Reader::is_finished() const
{
  // Your code here.
  return writer().is_closed() && !bytes_buffered();
}

uint64_t Reader::bytes_popped() const
{
  // Your code here.
  return _bytes_popped;
}

string_view Reader::peek() const
{
  // Your code here.
  return {};
}

void Reader::pop( uint64_t len )
{
  // Your code here.
  (void)len;
}

uint64_t Reader::bytes_buffered() const
{
  // Your code here.
  return _bytes_buffered;
}
