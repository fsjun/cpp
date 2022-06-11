#pragma once

#include "transport/TransportManager.h"

#define UDP_MAX_PACK_SIZE 65507 // 65535-20-8=65507 ip max size is 2^16-1=65535,ip headr size is 20 bytes, udp header size is 8 bytes.
#define UDP_MAX_MTU_SIZE 1472 // 1500-20-8=1472 data link layer MTU is 1500

class UdpTransport : public std::enable_shared_from_this<UdpTransport> {
public:
    UdpTransport(shared_ptr<boost::asio::io_context>& ioc);
    ~UdpTransport();
    int init(string host, int port);
    void close();
    string getLocalHost();
    int getLocalPort();

    void do_read(char* data, int size, boost::asio::ip::udp::endpoint& ep, std::function<void(const boost::system::error_code& err, const size_t& bytes)> cb);
    void do_write(char* data, int size, boost::asio::ip::udp::endpoint& ep, std::function<void(const boost::system::error_code& err, const size_t& bytes)> cb);

private:
    mutex mMutex;
    shared_ptr<boost::asio::io_context> mIoContext;
    shared_ptr<boost::asio::ip::udp::socket> mSock;
    string mLocalHost;
    int mLocalPort;
};
