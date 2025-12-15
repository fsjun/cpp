#pragma once

#include "transport/Transport.h"

#define UDP_MAX_PACK_SIZE 65507 // 65535-20-8=65507 ip max size is 2^16-1=65535,ip headr size is 20 bytes, udp header size is 8 bytes.
#define UDP_MAX_MTU_SIZE 1472 // 1500-20-8=1472 data link layer MTU is 1500

class UdpTransport : public std::enable_shared_from_this<UdpTransport>, public Transport {
public:
    class Listener {
    public:
        virtual void onRead(shared_ptr<UdpTransport> transport, shared_ptr<vector<char>> buff, boost::asio::ip::udp::endpoint& ep, const boost::system::error_code& err, const size_t& bytes) = 0;
        virtual void onWrite(shared_ptr<UdpTransport> transport, shared_ptr<vector<char>> buff, boost::asio::ip::udp::endpoint& ep, const boost::system::error_code& err, const size_t& bytes) = 0;
    };

    UdpTransport(shared_ptr<boost::asio::io_context>& ioc);
    ~UdpTransport();
    void setListener(weak_ptr<Listener> listener);
    int init(string host, int port);
    void close();
    string getLocalHost();
    int getLocalPort();

    void read(shared_ptr<vector<char>> buff);
    void write(shared_ptr<vector<char>> buff, boost::asio::ip::udp::endpoint ep);

private:
    void writePending();
    void doWrite(shared_ptr<vector<char>> buff, boost::asio::ip::udp::endpoint ep);
    
    void on_read(shared_ptr<vector<char>> buff, boost::asio::ip::udp::endpoint ep, const boost::system::error_code& err, const size_t& bytes);
    void on_write(shared_ptr<vector<char>> buff, boost::asio::ip::udp::endpoint ep, const boost::system::error_code& err, const size_t& bytes);

private:
    shared_ptr<boost::asio::ip::udp::socket> mSock;
    string mLocalHost;
    int mLocalPort = 0;

    std::list<std::pair<shared_ptr<vector<char>>, boost::asio::ip::udp::endpoint>> mPendingWrite;
    weak_ptr<Listener> mListener;
};
