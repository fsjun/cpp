#pragma once

#include "transport/TransportManager.h"

class TcpTransport : public std::enable_shared_from_this<TcpTransport> {
public:
    TcpTransport(shared_ptr<boost::asio::io_context>& ioc);
    virtual ~TcpTransport();
    int init(TransportTcpType type, string local_host, int local_port, string remote_host, int remote_port);
    void setCb(std::function<void(TransportTcpEventType type, shared_ptr<TcpTransport> transport)> cb);
    int getLocalPort();

    void do_read_some(char* data, int size, std::function<void(const boost::system::error_code& err, const size_t& bytes)> cb);
    void do_read(char* data, int size, std::function<void(const boost::system::error_code& err, const size_t& bytes)> cb);
    void async_read_until(boost::asio::streambuf& buffer, char delimiter, std::function<void(const boost::system::error_code& err, const size_t& bytes)> cb);
    void do_write_some(char* data, int size, std::function<void(const boost::system::error_code& err, const size_t& bytes)> cb);
    void do_write(char* data, int size, std::function<void(const boost::system::error_code& err, const size_t& bytes)> cb);

private:
    void do_connect(string host, int port);
    void on_connect(boost::system::error_code ec);
    void do_accept();
    void on_accept(shared_ptr<boost::asio::ip::tcp::socket> new_sock, boost::system::error_code ec);

private:
    mutex mMutex;
    TransportTcpType mTransportTcpType = TRANSPORT_TCP_TYPE_CLIENT;
    shared_ptr<boost::asio::io_context> mIoc;
    shared_ptr<boost::asio::ip::tcp::socket> mSock;
    shared_ptr<boost::asio::ip::tcp::acceptor> mAcceptor;

    int mLocalPort = 0;

    std::function<void(TransportTcpEventType type, shared_ptr<TcpTransport> transport)> mCb;
};
