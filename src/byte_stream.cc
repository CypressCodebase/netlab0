#include "byte_stream.hh"

using namespace std;

// Constructor for ByteStream class
ByteStream::ByteStream(uint64_t capacity) : 
    capacity_(capacity),
    bytes_pushed_(0),    // Initialize bytes_pushed_ to 0
    bytes_popped_(0),    // Initialize bytes_popped_ to 0 
    closed_(false),// Initialize buffer_ as an empty string
    buffer_()// Initialize closed_ as false
{
    // Constructor body (if needed)
}
bool Writer::is_closed() const
{
  return closed_;
}

void Writer::push( string data )
{
  // Your code here.
  //choose between the lowest number of bytes
  auto write_count = min( available_capacity(), data.size());
  
  if ( !write_count ) {
    return;
  }
  //add total bytes processed.
  
  bytes_pushed_ += write_count;
  //add pushed data to buffer
  buffer_ += data.substr( 0, write_count  );
  return;
}

void Writer::close()
{
  // Your code here.
  //close Bytestream.
  closed_ = true;
  return;
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
  return bytes_pushed_;
}

bool Reader::is_finished() const
{

  return writer().is_closed() && buffer_.empty();
}

uint64_t Reader::bytes_popped() const
{
  // Your code here.
  return bytes_popped_;
}

string_view Reader::peek() const
{
    return std::string_view(buffer_);
    }


void Reader::pop( uint64_t len )
{
  // Your code here.
  //ensure pop_count is a within constraint.
  auto pop_count = min( len, buffer_.size());
  //while bytes are still being processed
if ( !pop_count ) {
    return;
  }
  buffer_.erase(0, pop_count);
  bytes_popped_ += pop_count;
}

uint64_t Reader::bytes_buffered() const
{
  // Your code here.
  return buffer_.size();
}
