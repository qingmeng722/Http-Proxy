#include <httprequest.h>

HttpRequest::HttpRequest(const http::request<http::string_body>& req, const std::string& clientIp, int id)
{
    this->id = id;
    this->clientIp = clientIp;
    
    method = req.method_string().to_string();
    url = req.target().to_string();

    // 11 -> HTTP/1.1
    int ver = req.version();
    protocol = "HTTP/" + std::to_string(ver / 10) + "." + std::to_string(ver % 10);

    for (auto const& field : req) {
        headers[field.name_string().to_string()] = field.value().to_string();
    }

    if (method == "CONNECT") {
        // "CONNECT host:port HTTP/1.1"
        // url : "host:port"
        auto pos = url.find(':');
        if (pos != std::string::npos) {
            host = url.substr(0, pos);
            port = url.substr(pos + 1);
        } else {
            host = url;
            port = "";
        }
    } else {
        // get post
        auto it = headers.find("Host");
        if (it != headers.end()) {
            std::string hostPort = it->second;
            auto pos = hostPort.find(':');
            if (pos != std::string::npos) {
                host = hostPort.substr(0, pos);
                port = hostPort.substr(pos + 1);
            } else {
                host = hostPort;
                port = "";
            }
        }
    }

    body = req.body();

    requestLine = method + " " + url + " " + protocol;

    std::ostringstream oss;
    oss << requestLine << "\r\n";
    for (auto const& field : headers) {
        oss << field.first << ": " << field.second << "\r\n";
    }
    oss << "\r\n";
    oss << body;
    fullRequest = oss.str();
}


// 104: "GET www.bbc.co.uk/ HTTP/1.1" from 1.2.3.4 @ Sun Jan 1 22:58:17 2017
std::string HttpRequest::printRequest(){
    std::string timeStr = getUtcTime();
    std::string answer;
    answer = std::to_string(id) + ": " + "\"" + method + " " + host + " " + protocol 
        + "\" from " + clientIp + " @ " + timeStr;
    return answer;
}

std::string HttpRequest::getMethod(){
    return method;
}

std::string HttpRequest::getHost(){
    return host;
}

int HttpRequest::getId(){
    return id;
}

std::string HttpRequest::httpGetPost(){
    std::string portStr = port.empty() ? "80" : port;

    struct addrinfo hints, *res;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    int status = getaddrinfo(host.c_str(), portStr.c_str(), &hints, &res);
    if (status != 0) {
        std::cerr << "getaddrinfo error: " << gai_strerror(status) << std::endl;
        return "-1";
    }

    int sockfd = -1;
    struct addrinfo* p;
    for (p = res; p != nullptr; p = p->ai_next) {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1)
            continue;
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            continue;
        }
        break;
    }

    if (p == nullptr) {
        std::cerr << "Failed to connect to " << host << std::endl;
        freeaddrinfo(res);
        return "-1";
    }
    freeaddrinfo(res);

    headers["Connection"] = "close";
    if(headers.find("Proxy-Connection") != headers.end()){
        headers.erase("Proxy-Connection");
    }
    std::ostringstream requestStream;
    requestStream << method <<" " << url << " " << protocol <<"\r\n";
    for (const auto& header : headers) {
        requestStream << header.first << ": " << header.second << "\r\n";
    }
    requestStream << "\r\n";
    requestStream << body;
    std::string requestStr = requestStream.str();
    // std::cout << requestStr << std::endl;

    ssize_t totalSent = 0;
    ssize_t len = requestStr.size();
    while (totalSent < len) {
        ssize_t sent = send(sockfd, requestStr.c_str() + totalSent, len - totalSent, 0);
        if (sent == -1) {
            std::cerr << "send failed" << std::endl;
            close(sockfd);
            return "-1";
        }
        totalSent += sent;
    }

    const int bufSize = 100000;
    char buffer[bufSize];
    std::string response;
    ssize_t bytes;

    while((bytes = recv(sockfd, buffer, bufSize - 1, 0)) > 0){
        buffer[bytes] = '\0';
        response.append(buffer, bytes);
        // std::cout << "receive " << bytes << " from server" << std::endl;
    }

    if (bytes == -1) {
        std::cerr << "recv failed" << std::endl;
        close(sockfd);
        return "-1";        
    }

    close(sockfd);
    return response;
}


int HttpRequest::httpConnect(){
    std::string portStr = port.empty() ? "443" : port;

    struct addrinfo hints, *res;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    int status = getaddrinfo(host.c_str(), portStr.c_str(), &hints, &res);
    if (status != 0) {
        std::cerr << "getaddrinfo error: " << gai_strerror(status) << std::endl;
        return -1;
    }

    int sockfd = -1;
    struct addrinfo* p;
    for (p = res; p != nullptr; p = p->ai_next) {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1)
            continue;
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            continue;
        }
        break;
    }

    if (p == nullptr) {
        std::cerr << "Failed to connect to " << host << std::endl;
        freeaddrinfo(res);
        return -1;
    }
    freeaddrinfo(res);

    return sockfd;
}

std::string HttpRequest::getUrl(){
    return url;
}

bool HttpRequest::httpEqual(HttpRequest* httpRequest){
    if(httpRequest->getUrl() == url){
        return true;
    }
    return false;
}