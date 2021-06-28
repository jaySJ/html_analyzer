#include <GetUrlContent.hpp>

#include <cstdlib>
#include <iostream>
#include <string>
#include <memory>


// NOTE : the following root certificates is an example source from Boost.org 
// and is being used AS-IS, not intended for production/real use.
#include "root_certificates.hpp"

//#define HTTP_REQ_DEBUG

// overload stream operator
std::ostream & operator<<(std::ostream & os, const UrlReq & req)
{
    os << " Request details " 
            << " Protocol: "    << req._protocol << std::endl
            << " Port: "        << req._port     << std::endl
            << " Domain: "      << req._domain   << std::endl
            << " Resource: "    << req._resource << std::endl
            << " Query: "       << req._query    << std::endl;
    return os;
}

/**
 * Parses a url into constituents for constucting an http request object 
 * 
 * @param urlString full url path from where to fetch content
 * @param req       non-const reference to pre-allocated url request object
 *                  which will be populated upon return
 * @return void
 * 
 */
void parseUrl(const std::string& urlString, UrlReq & req) {

    std::cout << std::endl << "Input URL : " << urlString << std::endl;
    auto protocol_pos = urlString.find_first_of("://");
    // Get protocol name
    req._protocol = urlString.substr(0, protocol_pos);
    auto remString = urlString.substr(protocol_pos + 3);
    // Set port based on protocol
    req._port = (req._protocol.compare("https") == 0) ? "443" : "80";
    auto resource_pos = remString.find_first_of ('/');
    req._domain = remString.substr(0, resource_pos);
    auto query_pos = remString.find_first_of ('?');
    req._resource = (resource_pos != std::string::npos) ? remString.substr(resource_pos, query_pos - resource_pos) : "";
    req._query = (query_pos != std::string::npos) ? remString.substr(query_pos + 1) : "?";
}

/**
 * Parses a url into constituents for constucting an http request object 
 * 
 * @param urlString full url path from where to fetch content
 * @return a string containing the body/contents of the url GET request response 
 * 
 */
std::string UrlContentGetter::getUrlContent(const std::string & urlString)
{
    UrlReq url_req;
    parseUrl(urlString, url_req);
#ifdef HTTP_REQ_DEBUG
    std::cout << url_req << std::endl;
#endif
    const auto request_version = 11;    // Always assuming request version 11

    std::string retVal;
    try
    {
        // The io_context is required for all I/O
        boost::asio::io_context ioc;
        // The SSL context is required, and holds certificates
        ssl::context ctx{ssl::context::sslv23_client};

        // WARNING: UNSAFE - fake certificates in root_certificats.hpp.
        // are only for illustrative purposes. Please read the comments
        // there.

        // This holds the root certificate used for verification.
        // load_root_certificates(ctx);

        // Verify the remote server's certificate
        // ctx.set_verify_mode(ssl::verify_peer);

        // WARNING: UNSAFE - comment this out for production 
        ctx.set_verify_mode(ssl::verify_none);

        ///////////////////////////////////

        // Create a context that uses the default paths for
        // finding CA certificates.
        ctx.set_default_verify_paths();

        // Open a socket and connect it to the remote host.
        ssl_socket sock(ioc, ctx);

        tcp::resolver resolver(ioc);
        tcp::resolver::query query(url_req._domain, url_req._protocol);

        auto const results = resolver.resolve(query);
        boost::asio::connect(sock.lowest_layer(), results);
        sock.lowest_layer().set_option(tcp::no_delay(true));
        
        // Make the connection on the IP address we get from a lookup
        boost::asio::connect(sock.next_layer(), results.begin(), results.end());

        // Perform the SSL handshake
        sock.handshake(ssl::stream_base::client);

        // Set up an HTTP GET request message
        std::string verb = "GET";
        
        http::request<http::string_body> req;        
        req.method_string(verb);
        req.target(url_req._resource);
        req.version(request_version);
        
        using http::field;
        req.set(http::field::host, url_req._domain);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    
    #ifdef HTTP_REQ_DEBUG
        std::cout << "Request prepared " << req ;
    #endif

        // Send the HTTP request to the remote host
        http::write(sock, req);

        // This buffer is used for reading and must be persisted
        boost::beast::flat_buffer buffer;

        // Declare a container to hold the response
        http::response<http::dynamic_body> res;
    
        // Receive the HTTP response
        http::read(sock, buffer, res);

        retVal = boost::beast::buffers_to_string(res.body().data());
        std::cout << "Response size = " << retVal.size() << std::endl;

        // Shut down
        shutDownConnection(sock);
    }
    catch(std::exception const& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return retVal;
}

/**
 * Gracefully shutdown communication sockets
 * 
 */
void UrlContentGetter::shutDownConnection (ssl_socket & sock) {
    
    // Close the stream
    boost::system::error_code ec;

    // Cancel any outstanding operations so we can shutdown properly
    sock.lowest_layer().cancel(ec);
    
    sock.shutdown(ec);

    // Do shutdown 
    if ((ec.category() == boost::asio::error::get_ssl_category())
    /* && (ERR_GET_REASON(ec.value()) == SSL_R_SHORT_READ) */)
    {
        // Remote peer failed to send a close_notify message.
        std::cerr << "boost::asio::error::get_ssl_category value" << ec.value() << std::endl;
        throw boost::system::system_error{ec};
    }

    if(ec == boost::asio::error::eof)
    {
        // Rationale:
        // http://stackoverflow.com/questions/25587403/boost-asio-ssl-async-shutdown-always-finishes-with-an-error
        ec.assign(0, ec.category());
    }

    if(ec){
        std::cerr << "boost::asio::error::eof" << std::endl;
        throw boost::system::system_error{ec};
    }
    
}
