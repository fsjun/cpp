#include "HttpSession.h"
#include "osinfo/OsSignal.h"
#include <boost/format.hpp>

// Report a failure
static void fail(boost::system::error_code ec, char const* what)
{
    ERRLN("{}:{}\n", what, ec.message());
}

HttpSession::HttpSession(shared_ptr<boost::asio::io_context> ioc, shared_ptr<tcp::socket> socket, shared_ptr<HttpServer>& server)
    : stream_(std::move(*socket))
    , lambda_(*this)
{
    mIoContext = ioc;
    mServer = server;
    auto remote_endpoint = stream_.socket().remote_endpoint();
    mRemoteIp = remote_endpoint.address().to_string();
    mRemotePort = remote_endpoint.port();
}

HttpSession::~HttpSession()
{
    do_close();
}

void HttpSession::start()
{
    do_read();
    auto ioc = mIoContext;
    mServer->execute([ioc]() {
        ioc->run();
    });
}

void HttpSession::do_read()
{
    // Make the request empty before reading,
    // otherwise the operation behavior is undefined.
    req_ = {};
    // Read a request
    http::async_read(stream_, buffer_, req_, std::bind(&HttpSession::on_read, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
}

void HttpSession::on_read(boost::system::error_code ec, std::size_t bytes_transferred)
{
    boost::ignore_unused(bytes_transferred);
    // This means they closed the connection
    if (ec == http::error::end_of_stream) {
        return do_close();
    }
    if (ec) {
        return fail(ec, "read");
    }
    auto address = boost::str(boost::format("%s:%d") % mRemoteIp % mRemotePort);
    // Send the response
    handle_request(std::move(req_), lambda_, address, mServer);
}

void HttpSession::on_write(boost::system::error_code ec, std::size_t bytes_transferred, bool close)
{
    boost::ignore_unused(bytes_transferred);
    if (ec) {
        return fail(ec, "write");
    }
    if (close) {
        // This means we should close the connection, usually because
        // the response indicated the "Connection: close" semantic.
        return do_close();
    }
    // We're done with the response so delete it
    res_ = nullptr;
    // Read another request
    do_read();
}

void HttpSession::do_close()
{
    // Send a TCP shutdown
    boost::beast::error_code ec;
    stream_.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);

    // At this point the connection is closed gracefully
}
