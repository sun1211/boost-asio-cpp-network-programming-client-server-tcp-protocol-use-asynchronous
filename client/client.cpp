#include <boost/predef.h> // Tools to identify the OS.

// We need this to enable cancelling of I/O operations on
// Windows XP, Windows Server 2003 and earlier.
// Refer to "http://www.boost.org/doc/libs/1_58_0/
// doc/html/boost_asio/reference/basic_stream_socket/
// cancel/overload1.html" for details.
#ifdef BOOST_OS_WINDOWS
#define _WIN32_WINNT 0x0501

#if _WIN32_WINNT <= 0x0502 // Windows Server 2003 or earlier.
#define BOOST_ASIO_DISABLE_IOCP
#define BOOST_ASIO_ENABLE_CANCELIO
#endif
#endif

#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>

#include <thread>
#include <mutex>
#include <memory>
#include <list>
#include <iostream>

using namespace boost;

// Function pointer type that points to the callback
// function which is called when a request is complete.
// Based on the values of the parameters passed to it, it outputs information about the finished request.
typedef void (*Callback)(unsigned int request_id,        // unique identifier of the request is assigned to the request when it was initiated.
                         const std::string &response,    // the response data
                         const system::error_code &ec);  // error information

// data structure whose purpose is to keep the data related to a particular request while it is being executed
struct Session
{
    Session(asio::io_service &ios,
            const std::string &raw_ip_address,
            unsigned short port_num,
            const std::string &request,
            unsigned int id,
            Callback callback) : m_sock(ios),
                                 m_ep(asio::ip::address::from_string(raw_ip_address),
                                      port_num),
                                 m_request(request),
                                 m_id(id),
                                 m_callback(callback),
                                 m_was_cancelled(false) {}

    asio::ip::tcp::socket m_sock; // Socket used for communication
    asio::ip::tcp::endpoint m_ep; // Remote endpoint.
    std::string m_request;        // Request string.

    // streambuf where the response will be stored.
    asio::streambuf m_response_buf;
    std::string m_response; // Response represented as a string.

    // Contains the description of an error if one occurs during
    // the request lifecycle.
    system::error_code m_ec;

    unsigned int m_id; // Unique ID assigned to the request.

    // Pointer to the function to be called when the request
    // completes.
    Callback m_callback;

    bool m_was_cancelled;
    std::mutex m_cancel_guard;
};

// class that provides the asynchronous communication functionality.
class AsyncTCPClient : public boost::noncopyable
{
public:
    AsyncTCPClient(unsigned char num_of_threads)
    {

        //instantiates an object of the asio::io_service::work class
        // passing an instance of the asio::io_service class named m_ios to its constructor
        m_work.reset(new boost::asio::io_service::work(m_ios));

        for (unsigned char i = 1; i <= num_of_threads; i++)
        {
            //spawns a thread that calls the run() method of the m_ios object.
            std::unique_ptr<std::thread> th(
                new std::thread([this]()
                                { m_ios.run(); }));

            m_threads.push_back(std::move(th));
        }
    }
    // initiates a request to the server
    void emulateLongComputationOp(
        unsigned int duration_sec,          //represents the request parameter according to the application layer protocol
        const std::string &raw_ip_address,  //specify the server to which the request should be sent.
        unsigned short port_num,            //specify the server to which the request should be sent.
        Callback callback,                  //callback function, which will be called when the request is complete.
        unsigned int request_id)    // unique identifier of the request
    {

        // preparing a request string and allocating an instance of the Session structure
        // that keeps the data associated with the request including a socket object
        // that is used to communicate with the server.
        std::string request = "EMULATE_LONG_CALC_OP " + std::to_string(duration_sec) + "\n";
        std::shared_ptr<Session> session =
            std::shared_ptr<Session>(new Session(m_ios,
                                                 raw_ip_address,
                                                 port_num,
                                                 request,
                                                 request_id,
                                                 callback));

        //opened socket and the pointer to the Session object is added to the m_active_sessions map
        session->m_sock.open(session->m_ep.protocol());

        // Add new session to the list of active sessions so
        // that we can access it if the user decides to cancel
        // the corresponding request before it completes.
        // Because active sessions list can be accessed from
        // multiple threads, we guard it with a mutex to avoid
        // data corruption.
        std::unique_lock<std::mutex> lock(m_active_sessions_guard);
        m_active_sessions[request_id] = session;
        lock.unlock();

        //connect the socket to the server
        session->m_sock.async_connect(session->m_ep,
                                      [this, session](const system::error_code &ec)
                                      {
                                          //checking the error code passed to it as the ec argument
                                          if (ec.value() != 0)
                                          {
                                              //we store the ec value in the corresponding Session object,
                                              session->m_ec = ec;
                                              //call the class's onRequestComplete() private method passing the Session object to it as an argument
                                              onRequestComplete(session);
                                              //then return.
                                              return;
                                          }

                                          // lock the m_cancel_guard mutex (the member of the request descriptor object)
                                          std::unique_lock<std::mutex> cancel_lock(session->m_cancel_guard);

                                          //check whether the request has not been canceled yet. 
                                          if (session->m_was_cancelled)
                                          {
                                              onRequestComplete(session);
                                              return;
                                          }

                                          //If we see that the request has not been canceled
                                          //we initiate the next asynchronous operation calling the Boost.Asio free function async_write()
                                          // to send the request data to the server.
                                          asio::async_write(session->m_sock,
                                                            asio::buffer(session->m_request),
                                                            [this, session](const boost::system::error_code &ec,
                                                                            std::size_t bytes_transferred)
                                                            {
                                                                // check the error code
                                                                if (ec.value() != 0)
                                                                {
                                                                    session->m_ec = ec;
                                                                    onRequestComplete(session);
                                                                    return;
                                                                }

                                                                //// lock the m_cancel_guard mutex (the member of the request descriptor object)
                                                                std::unique_lock<std::mutex> cancel_lock(session->m_cancel_guard);

                                                                // check whether or not the request has been canceled. 
                                                                if (session->m_was_cancelled)
                                                                {
                                                                    onRequestComplete(session);
                                                                    return;
                                                                }

                                                                // initiate the next asynchronous operation—async_read_until()—in order to receive a response from the server
                                                                asio::async_read_until(session->m_sock,
                                                                                       session->m_response_buf,
                                                                                       '\n',
                                                                                       [this, session](const boost::system::error_code &ec,
                                                                                                       std::size_t bytes_transferred)
                                                                                       {
                                                                                           //checks the error code
                                                                                           if (ec.value() != 0)
                                                                                           {
                                                                                               session->m_ec = ec;
                                                                                           }
                                                                                           else
                                                                                           {
                                                                                               std::istream strm(&session->m_response_buf);
                                                                                               std::getline(strm, session->m_response);
                                                                                           }

                                                                                           // the AsyncTCPClient class's private method onRequestComplete() is called
                                                                                           // and the Session object is passed to it as an argument.
                                                                                           onRequestComplete(session);
                                                                                       });
                                                            });
                                      });
    };

    // cancels the previously initiated request designated by the request_id argument
    void cancelRequest(unsigned int request_id) //accepts an identifier of the request to be canceled as an argument.
    {
        std::unique_lock<std::mutex>
            lock(m_active_sessions_guard);

        //looking for the Session object corresponding to the specified request in the m_active_sessions map.
        auto it = m_active_sessions.find(request_id);
        if (it != m_active_sessions.end())
        {
            std::unique_lock<std::mutex>
                cancel_lock(it->second->m_cancel_guard);

            it->second->m_was_cancelled = true;
            it->second->m_sock.cancel();
        }
    }

    // blocks the calling thread until all the currently running requests complete and deinitializes the client.
    void close()
    {
        // Destroy work object. This allows the I/O threads to
        // exit the event loop when there are no more pending
        // asynchronous operations.
        m_work.reset(NULL);

        // Waiting for the I/O threads to exit.
        for (auto &thread : m_threads)
        {
            thread->join();
        }
    }

private:
    // method is called whenever the request completes with any result.
    void onRequestComplete(std::shared_ptr<Session> session)
    {
        // Shutting down the connection. This method may
        // fail in case socket is not connected. We don�t care
        // about the error code if this function fails.
        boost::system::error_code ignored_ec;

        session->m_sock.shutdown( asio::ip::tcp::socket::shutdown_both, ignored_ec);

        // Remove session form the map of active sessions.
        std::unique_lock<std::mutex>
            lock(m_active_sessions_guard);

        auto it = m_active_sessions.find(session->m_id);
        if (it != m_active_sessions.end())
            m_active_sessions.erase(it);

        lock.unlock();

        boost::system::error_code ec;

        if (session->m_ec.value() == 0 && session->m_was_cancelled)
            ec = asio::error::operation_aborted;
        else
            ec = session->m_ec;

        // Call the callback provided by the user.
        session->m_callback(session->m_id,
                            session->m_response, ec);
    };

private:
    asio::io_service m_ios;
    std::map<int, std::shared_ptr<Session>> m_active_sessions;
    std::mutex m_active_sessions_guard;
    std::unique_ptr<boost::asio::io_service::work> m_work;
    std::list<std::unique_ptr<std::thread>> m_threads;
};

// a function that will serve as a callback, which we'll pass to the AsyncTCPClient::emulateLongComputationOp() method
// It outputs the result of the request execution and the response message to the standard output stream if the request is completed successfully
void handler(unsigned int request_id,
             const std::string &response,
             const system::error_code &ec)
{
    if (ec.value() == 0)
    {
        std::cout << "Request #" << request_id
                  << " has completed. Response: "
                  << response << std::endl;
    }
    else if (ec == asio::error::operation_aborted)
    {
        std::cout << "Request #" << request_id
                  << " has been cancelled by the user."
                  << std::endl;
    }
    else
    {
        std::cout << "Request #" << request_id
                  << " failed! Error code = " << ec.value()
                  << ". Error message = " << ec.message()
                  << std::endl;
    }

    return;
}

int main()
{
    try
    {
        AsyncTCPClient client(4);

        // Here we emulate the user's behavior.

        // creates an instance of the AsyncTCPClient class and then calls its emulateLongComputationOp() method to initiate three asynchronous requests
        // User initiates a request with id 1.
        client.emulateLongComputationOp(10, "127.0.0.1", 3333, handler, 1);

        // Decides to exit the application.
        client.close();
    }
    catch (system::system_error &e)
    {
        std::cout << "Error occured! Error code = " << e.code()
                  << ". Message: " << e.what();

        return e.code().value();
    }

    return 0;
};
