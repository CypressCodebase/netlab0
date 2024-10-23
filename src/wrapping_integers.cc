#include "wrapping_integers.hh"
#include <algorithm>

using namespace std;

//
//
Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  // Your code here.
  //convert absolute sequence number, n
  //so you can add it to 32 bit initial seq number. z.p 
  return zero_point + (n % (1LL << 32));
  //tcp can only read 32 bit numbers.
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  //difference between current raw value and isn. 32
  uint32_t offset = raw_value_ - zero_point.raw_value_;
  // calculate absoluate seqno uint64 closest to checkpoint 64 to 32
  uint32_t wrapcount = checkpoint / ( 1LL << 32 );
  // checkpoint modulo 32. for comparison
  uint32_t checkpoint_offset = checkpoint % ( 1LL << 32 );
  
  //2^31 is half of 32
  //compare the offsets, 
  //if the difference is greater than 2^31
  //not the candidate.
  //either offset maybe bigger than the other.
  if ( max( offset, checkpoint_offset ) - min( offset, checkpoint_offset ) > ( 1LL << 31 ) ) {
    
    //identify if the current offset is 1 wrap above or below.
    if ( checkpoint_offset > offset ) {
      wrapcount += 1;
    } else if ( wrapcount ) {
      wrapcount -= 1;
    }
  }
  //2^32 bits * cycles. + offset value in 64 bits
  return ( 1UL << 32 ) * wrapcount + offset;
}

