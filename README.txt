Compile both the server and the client bu running 'make' or 'make all' in this directory.
The client and server can also be compiled separately by running 'make' or 'make all' in their respective directories.

Run the server using './QRServer' in the 'server' directory, and run the client using './client' int the 'client' directory

Currently, the client sends four requests as fast as possible. This demonstrates the ability of the server to handle multiple
requests from one client and shows that the server refuses a request if it exceeds the rate limit.

The server uses the command line arguments PORT, RATE, MAX_USERS, and TIME_OUT as described in the project specification. Without
these arguments, they default to the values in the project specification.

The client can be called in three ways:
    ./client
    ./client <ip>
    ./client <ip> <port>
If the ip is missing, it defaults to 127.0.0.1. If the port is missing, it defaults to 2012.