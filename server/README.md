# Implementing an asynchronous TCP server

An asynchronous TCP server is a part of a distributed application that satisfies the following criteria:

- Acts as a server in the client-server communication model
- Communicates with client applications over TCP protocol
- Uses the asynchronous I/O and control operations
- May handle multiple clients simultaneously

A typical asynchronous TCP server works according to the following algorithm:
- Allocate an acceptor socket and bind it to a particular TCP port.
- Initiate the asynchronous accept operation.
- Spawn one or more threads of control and add them to the pool of threads that run the Boost.Asio event loop.
- When the asynchronous accept operation completes, initiate a new one to accept the next connection request.
- Initiate the asynchronous reading operation to read the request from the connected client.
- When the asynchronous reading operation completes, process the request and prepare the response message.
- Initiate the asynchronous writing operation to send the response message to the client.
- When the asynchronous writing operation completes, close the connection and deallocate the socket.
