# Chat server and clients

These two files act as a chat room program.
The server is what each client connects to. The server has
one main listening port to which is awaits any new connections.
When it receives a new connection it adds it to a list. Every loop,
it checks if it received any input on any of the fds. If it did,
then it forwards it to the appropriate client with the matching id.
If there are no clients with the matching id, it sends to all.

The client connects to the server and then receives and sends a
message to a random other host in a random amount of time.


For both program, non-blocking ports are used.
