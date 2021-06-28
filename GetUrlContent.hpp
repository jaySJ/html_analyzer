#pragma once

#include <string>
#include <memory>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/algorithm/string.hpp>

// Simple data structure for url request fields
struct UrlReq {
  std::string _protocol;  
  std::string _port;
  std::string _domain;
  std::string _resource; // everything after '/', possibly nothing
  std::string _query;    // everything after '?', possibly nothing
};

namespace beast = boost::beast;     // from <boost/beast.hpp>
namespace http = beast::http;       // from <boost/beast/http.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
namespace net = boost::asio;        // from <boost/asio.hpp>
using tcp = net::ip::tcp;           // from <boost/asio/ip/tcp.hpp>
using boost::asio::ip::tcp;
namespace ssl = boost::asio::ssl;
typedef ssl::stream<tcp::socket> ssl_socket;

class UrlContentGetter final {
public:
    UrlContentGetter() {}
    virtual ~UrlContentGetter() {}
    std::string getUrlContent(const std::string & urlString);
    std::string getContent() const { return m_content;}
    void shutDownConnection (ssl_socket & sock);
    
private:
    UrlReq m_url_req;
    std::string m_content;
};
