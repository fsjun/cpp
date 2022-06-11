#include "WebSocketServer.h"
#include "WsServerListener.h"

WebSocketServer::WebSocketServer(shared_ptr<boost::asio::io_context> ioContext, string host, int port, bool sslEnable, string caPath, string keyPath, string certPath, WebSocketServerCb cb)
{
    mIoContext = ioContext;
    mHost = std::move(host);
    mPort = port;
    mSslEnable = sslEnable;
    mCaPath = std::move(caPath);
    mKeyPath = std::move(keyPath);
    mCertPath = std::move(certPath);
    mCb = cb;
}

int WebSocketServer::start()
{
    if (mSslEnable) {
        // The SSL context is required, and holds certificates
        ssl::context ctx { ssl::context::sslv23 };
        ctx.set_options(boost::asio::ssl::context::default_workarounds | boost::asio::ssl::context::no_sslv2);
        ctx.load_verify_file(mCaPath);
        ctx.use_private_key_file(mKeyPath, boost::asio::ssl::context::file_format::pem);
        ctx.use_certificate_file(mCertPath, boost::asio::ssl::context::file_format::pem);
        // Create and launch a listening port
        mWebSocketListener = make_shared<WsServerListener>(mIoContext, ctx, mSslEnable, mHost, mPort, mCb);
    } else {
        ssl::context ctx { ssl::context::sslv23 };
        // Create and launch a listening port
        mWebSocketListener = make_shared<WsServerListener>(mIoContext, ctx, mSslEnable, mHost, mPort, mCb);
    }
    return mWebSocketListener->start();
}
