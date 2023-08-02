Compile both the server and the client bu running 'make' or 'make all' in this directory.
The client and server can also be compiled separately by running 'make' or 'make all' in their respective directories.

Run the server using './QRServer' in the 'server' directory, and run the client using './client' int the 'client' directory.

The client sends four requests as fast as possible. This demonstrates the ability of the server to handle multiple requests from one client and shows that the server refuses a request if it exceeds the rate limit.

The server accepts up to four command line arguments: PORT, RATE, MAX_USERS, and TIME_OUT.

    ./QRServer [PORT <portnum>] [RATE <request_limit> <time_in_seconds>] [MAX_USERS <maxusers>] [TIME_OUT <time_out_in_seconds>]

Without these arguments, they take on default values:

    PORT 2012
    RATE 3 60
    MAX_USERS 3
    TIME_OUT 80

The client accepts up to two command line arguments:

    ./client [ip] [port]

If the ip is missing, it defaults to 127.0.0.1. If the port is missing, it defaults to 2012.
