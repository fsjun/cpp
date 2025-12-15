#pragma once

#include "transport/Transport.h"

class TcpTransport : public std::enable_shared_from_this<TcpTransport>, public Transport {
public:
    class Listener {
    public:
        virtual void onAccept(shared_ptr<TcpTransport> transport, const boost::system::error_code& err) {}
        virtual void onConnect(shared_ptr<TcpTransport> transport, const boost::system::error_code& err) {}
        virtual void onRead(shared_ptr<TcpTransport> transport, shared_ptr<vector<char>> buff, const boost::system::error_code& err, const size_t& bytes) = 0;
        virtual void onWrite(shared_ptr<TcpTransport> transport, shared_ptr<vector<char>> buff, const boost::system::error_code& err, const size_t& bytes) = 0;
    };

    TcpTransport(shared_ptr<boost::asio::io_context>& ioc);
    virtual ~TcpTransport();
    void setListener(weak_ptr<Listener> listener);
    int init(bool isClient, string local_host, int local_port, string remote_host, int remote_port);
    int getLocalPort();

    void do_read_some(shared_ptr<vector<char>> buff);
    void do_read(shared_ptr<vector<char>> buff);
    void async_read_until(shared_ptr<vector<char>> buff, char delimiter);
    void do_write_some(shared_ptr<vector<char>> buff);
    void write(shared_ptr<vector<char>> buff);

private:
    void writePending();
    void doWrite(shared_ptr<vector<char>> buff);
    void do_connect(string host, int port);
    void on_connect(boost::system::error_code ec);
    void do_accept();
    void on_accept(shared_ptr<boost::asio::ip::tcp::socket> new_sock, boost::system::error_code ec);
    void on_read(shared_ptr<vector<char>> buff, const boost::system::error_code& err, const size_t& bytes);
    void on_read_until(shared_ptr<vector<char>> buff, char delimiter, const boost::system::error_code& err, const size_t& bytes);
    void on_write(shared_ptr<vector<char>> buff, const boost::system::error_code& err, const size_t& bytes);

private:
    shared_ptr<boost::asio::ip::tcp::socket> mSock;
    shared_ptr<boost::asio::ip::tcp::acceptor> mAcceptor;
    boost::asio::streambuf mStreambuffer;
    int mLocalPort = 0;

    std::list<shared_ptr<vector<char>>> mPendingWrite;
    weak_ptr<Listener> mListener;
};
