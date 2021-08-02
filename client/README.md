# Implementing an asynchronous TCP client

An asynchronous TCP client application supporting the asynchronous execution of the requests and request canceling functionality:
- Input from the user should be processed in a separate thread—the user interface thread. This thread should never be blocked for a noticeable amount of time.
- The user should be able to issue multiple requests to different servers.
- The user should be able to issue a new request before the previously issued requests complete.
- The user should be able to cancel the previously issued requests before they complete.