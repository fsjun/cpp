﻿#include "httpclient/HttpSyncClientSslSession.h"
#include "boost/beast/core/error.hpp"
#include "boost/beast/http/verb.hpp"
#include "boost/format.hpp"
#include "httpclient/HttpSyncClient.h"
#include "osinfo/NetworkInfo.h"
#include "url/Url.h"
#include <memory>
#include <string>

HttpSyncClientSslSession::~HttpSyncClientSslSession()
{
    this->close();
}

void HttpSyncClientSslSession::setHost(string val)
{
    mHost = val;
}

void HttpSyncClientSslSession::setPort(int val)
{
    mPort = val;
}

int HttpSyncClientSslSession::connect()
{
    string port = std::to_string(mPort);
    ssl::context ctx(ssl::context::tlsv12_client);
    load_root_certificates(ctx);
    // Verify the remote server's certificate
    ctx.set_verify_mode(ssl::verify_none);

    tcp::resolver resolver(mIoc);
    boost::system::error_code ec;
    auto const results = resolver.resolve(mHost.c_str(), port.c_str(), ec);
    if (ec) {
        ERRLN("resolve error, host:{} port:{} msg:{}", mHost, port, ec.message());
        return -1;
    }
    mStream = std::make_unique<beast::ssl_stream<beast::tcp_stream>>(mIoc, ctx);
    // Set SNI Hostname (many hosts need this to handshake successfully)
    if (!SSL_set_tlsext_host_name(mStream->native_handle(), mHost.c_str())) {
        beast::error_code ec { static_cast<int>(::ERR_get_error()), net::error::get_ssl_category() };
        ERRLN("SSL_set_tlsext_host_name error, host:{} port:{} msg:{}", mHost, port, ec.message());
        return -1;
    }
    beast::get_lowest_layer(*mStream).connect(results, ec);
    if (ec) {
        ERRLN("connect error, host:{} port:{} msg:{}", mHost, port, ec.message());
        return -1;
    }
    // Perform the SSL handshake
    mStream->handshake(ssl::stream_base::client, ec);
    if (ec) {
        ERRLN("handshake error, host:{} port:{} msg:{}", mHost, port, ec.message());
        return -1;
    }
    return 0;
}

void HttpSyncClientSslSession::close()
{
    if (!mStream) {
        return;
    }
    beast::error_code ec;
    mStream->shutdown(ec);
    if (ec) {
        ERRLN("close error, host:{} port:{} msg:{}", mHost, mPort, ec.message());
    }
    mStream.reset();
}

int HttpSyncClientSslSession::httpGet(string path, vector<map<string, string>> headers, string& body)
{
    // Set up an HTTP GET request message
    http::request<http::string_body> req { http::verb::get, path, 11 };
    req.set(http::field::host, mHost);
    req.set(http::field::user_agent, HttpSyncClient::GetUserAgent());
    for (auto headerMap : headers) {
        for (auto val : headerMap) {
            req.set(val.first, val.second);
        }
    }

    // Send the HTTP request to the remote host
    http::write(*mStream, req);

    // This buffer is used for reading and must be persisted
    beast::flat_buffer buffer;

    // Declare a container to hold the response
    http::response<http::string_body> res;

    // Receive the HTTP response
    http::read(*mStream, buffer, res);

    int code = res.result_int();
    string reason = res.reason();
    if (code / 100 != 2) {
        ERRLN("http fail, path:{} code:{} reason:{}", path, code, reason);
        return -1;
    }
    body = res.body();
    INFOLN("http success, path:{} code:{} reason:{} body:{}", path, code, reason, body);
    return 0;
}

int HttpSyncClientSslSession::httpPost(string path, vector<map<string, string>> headers, string contentType, string content, string& body)
{
    // Set up an HTTP POST request message
    http::request<http::string_body> req { http::verb::post, path, 11 };
    req.set(http::field::host, mHost);
    req.set(http::field::user_agent, HttpSyncClient::GetUserAgent());
    for (auto headerMap : headers) {
        for (auto val : headerMap) {
            req.set(val.first, val.second);
        }
    }
    if (!contentType.empty()) {
        req.set(http::field::content_type, contentType);
    }
    if (!content.empty()) {
        req.body() = content;
        req.prepare_payload();
    }

    // Send the HTTP request to the remote host
    http::write(*mStream, req);

    // This buffer is used for reading and must be persisted
    beast::flat_buffer buffer;

    // Declare a container to hold the response
    http::response<http::string_body> res;

    // Receive the HTTP response
    http::read(*mStream, buffer, res);

    int code = res.result_int();
    string reason = res.reason();
    if (code / 100 != 2) {
        ERRLN("http fail, path:{} code:{} reason:{}", path, code, reason);
        return -1;
    }
    body = res.body();
    INFOLN("http success, path:{} content:{} code:{} reason:{} body:{}", path, content, code, reason, body);
    return 0;
}

int HttpSyncClientSslSession::httpMethod(string path, boost::beast::http::verb method, vector<map<string, string>> headers, string contentType, string content, string& body)
{
    // Set up an HTTP POST request message
    http::request<http::string_body> req { method, path, 11 };
    req.set(http::field::host, mHost);
    req.set(http::field::user_agent, HttpSyncClient::GetUserAgent());
    for (auto headerMap : headers) {
        for (auto val : headerMap) {
            req.set(val.first, val.second);
        }
    }
    if (!contentType.empty()) {
        req.set(http::field::content_type, contentType);
    }
    if (!content.empty()) {
        req.body() = content;
        req.prepare_payload();
    }

    // Send the HTTP request to the remote host
    http::write(*mStream, req);

    // This buffer is used for reading and must be persisted
    beast::flat_buffer buffer;

    // Declare a container to hold the response
    http::response<http::string_body> res;

    // Receive the HTTP response
    http::read(*mStream, buffer, res);

    int code = res.result_int();
    string reason = res.reason();
    if (code / 100 != 2) {
        ERRLN("http fail, path:{} code:{} reason:{}", path, code, reason);
        return -1;
    }
    body = res.body();
    INFOLN("http success, path:{} content:{} code:{} reason:{} body:{}", path, content, code, reason, body);
    return 0;
}

void HttpSyncClientSslSession::load_root_certificates(ssl::context& ctx)
{
    boost::system::error_code ec;
    std::string const cert =
        /*  This is the DigiCert root certificate.

                CN = DigiCert High Assurance EV Root CA
                OU = www.digicert.com
                O = DigiCert Inc
                C = US

                Valid to: Sunday, ?November ?9, ?2031 5:00:00 PM

                Thumbprint(sha1):
                5f b7 ee 06 33 e2 59 db ad 0c 4c 9a e6 d3 8f 1a 61 c7 dc 25
            */
        "-----BEGIN CERTIFICATE-----\n"
        "MIIDxTCCAq2gAwIBAgIQAqxcJmoLQJuPC3nyrkYldzANBgkqhkiG9w0BAQUFADBs\n"
        "MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n"
        "d3cuZGlnaWNlcnQuY29tMSswKQYDVQQDEyJEaWdpQ2VydCBIaWdoIEFzc3VyYW5j\n"
        "ZSBFViBSb290IENBMB4XDTA2MTExMDAwMDAwMFoXDTMxMTExMDAwMDAwMFowbDEL\n"
        "MAkGA1UEBhMCVVMxFTATBgNVBAoTDERpZ2lDZXJ0IEluYzEZMBcGA1UECxMQd3d3\n"
        "LmRpZ2ljZXJ0LmNvbTErMCkGA1UEAxMiRGlnaUNlcnQgSGlnaCBBc3N1cmFuY2Ug\n"
        "RVYgUm9vdCBDQTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAMbM5XPm\n"
        "+9S75S0tMqbf5YE/yc0lSbZxKsPVlDRnogocsF9ppkCxxLeyj9CYpKlBWTrT3JTW\n"
        "PNt0OKRKzE0lgvdKpVMSOO7zSW1xkX5jtqumX8OkhPhPYlG++MXs2ziS4wblCJEM\n"
        "xChBVfvLWokVfnHoNb9Ncgk9vjo4UFt3MRuNs8ckRZqnrG0AFFoEt7oT61EKmEFB\n"
        "Ik5lYYeBQVCmeVyJ3hlKV9Uu5l0cUyx+mM0aBhakaHPQNAQTXKFx01p8VdteZOE3\n"
        "hzBWBOURtCmAEvF5OYiiAhF8J2a3iLd48soKqDirCmTCv2ZdlYTBoSUeh10aUAsg\n"
        "EsxBu24LUTi4S8sCAwEAAaNjMGEwDgYDVR0PAQH/BAQDAgGGMA8GA1UdEwEB/wQF\n"
        "MAMBAf8wHQYDVR0OBBYEFLE+w2kD+L9HAdSYJhoIAu9jZCvDMB8GA1UdIwQYMBaA\n"
        "FLE+w2kD+L9HAdSYJhoIAu9jZCvDMA0GCSqGSIb3DQEBBQUAA4IBAQAcGgaX3Nec\n"
        "nzyIZgYIVyHbIUf4KmeqvxgydkAQV8GK83rZEWWONfqe/EW1ntlMMUu4kehDLI6z\n"
        "eM7b41N5cdblIZQB2lWHmiRk9opmzN6cN82oNLFpmyPInngiK3BD41VHMWEZ71jF\n"
        "hS9OMPagMRYjyOfiZRYzy78aG6A9+MpeizGLYAiJLQwGXFK3xPkKmNEVX58Svnw2\n"
        "Yzi9RKR/5CYrCsSXaQ3pjOLAEFe4yHYSkVXySGnYvCoCWw9E1CAx2/S6cCZdkGCe\n"
        "vEsXCS+0yx5DaMkHJ8HSXPfqIbloEpw8nL+e/IBcm2PN7EeqJSdnoDfzAIJ9VNep\n"
        "+OkuE6N36B9K\n"
        "-----END CERTIFICATE-----\n"

        "-----BEGIN CERTIFICATE-----\n"
        "MIIDaDCCAlCgAwIBAgIJAO8vBu8i8exWMA0GCSqGSIb3DQEBCwUAMEkxCzAJBgNV\n"
        "BAYTAlVTMQswCQYDVQQIDAJDQTEtMCsGA1UEBwwkTG9zIEFuZ2VsZXNPPUJlYXN0\n"
        "Q049d3d3LmV4YW1wbGUuY29tMB4XDTE3MDUwMzE4MzkxMloXDTQ0MDkxODE4Mzkx\n"
        "MlowSTELMAkGA1UEBhMCVVMxCzAJBgNVBAgMAkNBMS0wKwYDVQQHDCRMb3MgQW5n\n"
        "ZWxlc089QmVhc3RDTj13d3cuZXhhbXBsZS5jb20wggEiMA0GCSqGSIb3DQEBAQUA\n"
        "A4IBDwAwggEKAoIBAQDJ7BRKFO8fqmsEXw8v9YOVXyrQVsVbjSSGEs4Vzs4cJgcF\n"
        "xqGitbnLIrOgiJpRAPLy5MNcAXE1strVGfdEf7xMYSZ/4wOrxUyVw/Ltgsft8m7b\n"
        "Fu8TsCzO6XrxpnVtWk506YZ7ToTa5UjHfBi2+pWTxbpN12UhiZNUcrRsqTFW+6fO\n"
        "9d7xm5wlaZG8cMdg0cO1bhkz45JSl3wWKIES7t3EfKePZbNlQ5hPy7Pd5JTmdGBp\n"
        "yY8anC8u4LPbmgW0/U31PH0rRVfGcBbZsAoQw5Tc5dnb6N2GEIbq3ehSfdDHGnrv\n"
        "enu2tOK9Qx6GEzXh3sekZkxcgh+NlIxCNxu//Dk9AgMBAAGjUzBRMB0GA1UdDgQW\n"
        "BBTZh0N9Ne1OD7GBGJYz4PNESHuXezAfBgNVHSMEGDAWgBTZh0N9Ne1OD7GBGJYz\n"
        "4PNESHuXezAPBgNVHRMBAf8EBTADAQH/MA0GCSqGSIb3DQEBCwUAA4IBAQCmTJVT\n"
        "LH5Cru1vXtzb3N9dyolcVH82xFVwPewArchgq+CEkajOU9bnzCqvhM4CryBb4cUs\n"
        "gqXWp85hAh55uBOqXb2yyESEleMCJEiVTwm/m26FdONvEGptsiCmF5Gxi0YRtn8N\n"
        "V+KhrQaAyLrLdPYI7TrwAOisq2I1cD0mt+xgwuv/654Rl3IhOMx+fKWKJ9qLAiaE\n"
        "fQyshjlPP9mYVxWOxqctUdQ8UnsUKKGEUcVrA08i1OAnVKlPFjKBvk+r7jpsTPcr\n"
        "9pWXTO9JrYMML7d+XRSZA1n3856OqZDX4403+9FnXCvfcLZLLKTBvwwFgEFGpzjK\n"
        "UEVbkhd5qstF6qWK\n"
        "-----END CERTIFICATE-----\n";
    ;
    ctx.add_certificate_authority(boost::asio::buffer(cert.data(), cert.size()), ec);
    if (ec) {
        return;
    }
}
