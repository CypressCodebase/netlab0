//current code 
#include "reassembler.hh"

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  // Your code here.
  (void)first_index;
  (void)data;
  (void)is_last_substring;
  current_index_ = output_.writer().bytes_pushed();
  end_index_= first_index + data.size();
  byte_length_ = data.size();
  
  // CHECK IF CLOSED
  if ( output_.writer().is_closed() ) {
    return;
  }
  
  //if start of segment is greater than curr_index + available capacity.
  //this segment doesn't fit so remoove
  if ( first_index >= output_.writer().bytes_pushed() + output_.writer().available_capacity()) {
    return;
  }
  
  //CHECK IF EMPTY, and IF EMPTY&&LAST STRING, CLOSE.
  if ( data.empty() ) {
    if ( is_last_substring ) {
      output_.writer().close();
    }
    return;
  }
  //set last byte
  if ( is_last_substring ){
    final_index_ = first_index + data.size();
  }
  
  //If segment-end is before current byte counter. disregard.
  if ( end_index_ <= output_.writer().bytes_pushed() ) {
    return;
  }
  //==============================================================================
  //overlaps
  if ( first_index < output_.writer().bytes_pushed() ) {
    //overlap detected, push only extra.
    output_.writer().push( data.substr( output_.writer().bytes_pushed() - first_index ) );
    current_index_ = output_.writer().bytes_pushed();
    //if you pushed the last string
    push_unassembled( output_.writer(), first_index + data.size() );
    if ( is_last_substring ) {
      if ( first_index + data.size() == output_.writer().bytes_pushed() ) {
        unass_strings_.clear();
        output_.writer().close();
        return;
      }
    } 
    return;
  } 
  //==============================================================================
  if ( !output_.writer().available_capacity() ) {
    return;
  }
  //==============================================================================
  //if current match push data
  if( first_index == output_.writer().bytes_pushed()){
    output_.writer().push( data );
    current_index_ = output_.writer().bytes_pushed();
    if(is_last_substring) {
      output_.writer().close();
      return;
      }
    push_unassembled(output_.writer(), current_index_);
  }  
  if ( is_last_substring ) {
      if ( first_index + data.size() == output_.writer().bytes_pushed() ) {
        unass_strings_.clear();
        output_.writer().close();
        return;
      }
    } 
      //==============================================================================
    //REDUNDANT STRING CHECKER
if ( unass_strings_.contains( first_index )
      //AND SMALLER THAN EXISTING BUFFERED SEGMENT
       && unass_strings_.at( first_index ).size() 
       >= data.size() ) {
        //DISREGARD
    return;
  }
  //==============================================================================
  //unassembled pipeline
  //store as a map pair of first_index and data.
  if (first_index > output_.writer().bytes_pushed()) {
    // This segment is out of order (there's a gap), so we need to store map_index
    if (end_index_ < output_.writer().bytes_pushed() + output_.writer().available_capacity()) {
        // The entire segment can fit within available capacity, store map_index
        unass_strings_[first_index] = std::move(data);
    } else {
        // Truncate the segment to fit within available capacity

        unass_strings_[first_index] 
        = data.substr(0, 
        output_.writer().available_capacity() + output_.writer().bytes_pushed() - first_index);
    }
    return;
  }

  //==============================================================================
  //future strings that can be stored
  //if in capacity length then store
  //if out of capacity, truncate and store 
  if ( 
  end_index_ - output_.writer().bytes_pushed() <= output_.writer().available_capacity() ) 
  {  
    unass_strings_[first_index] = std::move( data );
    return;
  } 
  else {
    unass_strings_[first_index]
      = data.substr( 0, output_.writer().available_capacity() 
      + output_.writer().bytes_pushed() - first_index );
      return;
  }
}

uint64_t Reassembler::bytes_pending() const {
    size_t total_pending_bytes = 0;
    size_t next_expected_byte = output_.writer().bytes_pushed();

    for (const auto& [segment_start, segment_data] : unass_strings_) {
        size_t segment_end = segment_start + segment_data.size();

        // If the current segment starts after the next expected byte, count the whole segment
        if (segment_start >= next_expected_byte) {
            total_pending_bytes += segment_data.size();
            next_expected_byte = segment_end;
        }
        // If the current segment overlaps with previously known bytes, count only the new portion
        else if (segment_end > next_expected_byte) {
            total_pending_bytes += segment_end - next_expected_byte;
            next_expected_byte = segment_end;
        }
    }

    return total_pending_bytes;
}


//for every key pair in map
//check if key = current index.
///if so push, set new index.
//check if there is overlap 
///push truncating string.
void Reassembler::push_unassembled(Writer& writer, size_t index) {
    size_t curr_index = index;
    

  for ( const auto& map_index : unass_strings_) {
    if ( map_index.first > writer.bytes_pushed() ) {
      break;
    }
    if ( map_index.first >= curr_index ) {
      curr_index += map_index.second.size();
      writer.push( map_index.second );
      if(writer.bytes_pushed() == final_index_){
        writer.close();
      }
    } 
    
    else {
      if ( map_index.first + map_index.second.size() > curr_index ) {
        writer.push( map_index.second.substr( curr_index - map_index.first ) );
      }
      curr_index = max( curr_index, map_index.first + map_index.second.size() );
    }
    if(writer.bytes_pushed() == final_index_){
        writer.close();
      }
  }
  if(writer.bytes_pushed() == final_index_){
        writer.close();
      }
}

