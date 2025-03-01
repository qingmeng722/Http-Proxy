#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H

#include <boost/beast/http.hpp>
#include <map>
#include <sstream>
#include <string>
#include <utctime.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <iostream>

namespace beast = boost::beast;
namespace http = beast::http;

class HttpRequest{
private:
    std::map<std::string, std::string> headers;
    std::string method;
    std::string url;    //connect
    std::string protocol;
    std::string body;
    std::string host;   //get post
    std::string port;   //get post
    std::string requestLine;
    std::string fullRequest;
    std::string clientIp;
    int id;

public:
    HttpRequest(const http::request<http::string_body>& req, const std::string& clientIp, int id);

    std::string printRequest();

    std::string getMethod();

    std::string getHost();

    int getId();

    std::string httpGetPost();

    int httpConnect();

    std::string getUrl();

    bool httpEqual(HttpRequest* httpRequest);
};

#endif