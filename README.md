# P2P
Basic peer-to-peer node implemented in C with pthreads. The type of socket used is `SOCK_STREAM` for a reliable, two-way communication using TCP. The server can only handle one client at a time. Adding `select()` to the server function is the ideal way to achieve single-threaded, synchronous communication with multiple clients.

To build: `gcc -Wall -Werror -o p2p p2p.c -lpthread`
