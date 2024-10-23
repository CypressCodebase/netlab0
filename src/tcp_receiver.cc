#include "tcp_receiver.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  //record RST flag
  if(message.RST){
    error = true;
    reassembler_.reader().set_error();
    return;
  }

  //if SYN flag, set initial seqno to current seqno.
  if ( message.SYN ) {
    initial_seqno = Wrap32 { message.seqno };
    connected = true;
  }

  //SYN takes 1 seqno, if SYN is false it will = 1
    uint64_t const absolute_index = 
      message.seqno.unwrap( initial_seqno, writer().bytes_pushed() ) - !message.SYN;
    
    reassembler_.insert( absolute_index,message.payload,message.FIN );
}

TCPReceiverMessage TCPReceiver::send() const
{
  // Your code here
 TCPReceiverMessage message;

  //set RST flag
  if(writer().has_error()){
    message.RST = true;
  }
 
 //check connection, ackno is turned into 32bit
 if(connected){
  message.ackno = Wrap32::wrap(writer().bytes_pushed(), initial_seqno) + 1 + writer().is_closed();
  }
 
 //match window size depending on current conditions
 if ( writer().available_capacity() < UINT16_MAX ) {
    message.window_size = writer().available_capacity();
  } else {
    message.window_size = UINT16_MAX;
  }
  return message;
}
