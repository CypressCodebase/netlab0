#include "socket.hh"
#include "tcp_minnow_socket.hh"
#include <cstdlib>
#include <iostream>
#include <span>
#include <string>

using namespace std;

void get_URL( const string& host, const string& path )
{  
  CS144TCPSocket socket;
  
  //Write here I guess
  //TCP and Address classes
    //Resolve ip from address 
    Address server_address(host, "http");
    //make a socket connecting to that server
    socket.connect(server_address);
    //make a http request 
    const string get_request = "GET "+path+" HTTP/1.1\r\n"
            "Host: "+host +"\r\n"
            "Connection: close\r\n\r\n";
    //send http request
    socket.write(get_request);
    //receive information from said socket
    string response;
    string buffer;
    //put that into a buffer
    while (!socket.eof()){
    socket.read( buffer );
    response += buffer;
    }
  
    std::cout << response;
  //print that buffer
  
  socket.shutdown(2);
}

int main( int argc, char* argv[] )
{
  try {
    if ( argc <= 0 ) {
      abort(); // For sticklers: don't try to access argv[0] if argc <= 0.
    }

    auto args = span( argv, argc );

    // The program takes two command-line arguments: the hostname and "path" part of the URL.
    // Print the usage message unless there are these two arguments (plus the program name
    // itself, so arg count = 3 in total).
    if ( argc != 3 ) {
      cerr << "Usage: " << args.front() << " HOST PATH\n";
      cerr << "\tExample: " << args.front() << " stanford.edu /class/cs144\n";
      return EXIT_FAILURE;
    }

    // Get the command-line arguments.
    const string host { args[1] };
    const string path { args[2] };

    // Call the student-written function.
    get_URL( host, path );
  } catch ( const exception& e ) {
    cerr << e.what() << "\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
