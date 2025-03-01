#include <httpresponse.h>

// Removes whitespace at both ends of the string
std::string HttpResponse::trim(const std::string& str) {
    const std::string whitespace = " \t\r\n";
    size_t start = str.find_first_not_of(whitespace);
    size_t end = str.find_last_not_of(whitespace);
    return (start == std::string::npos) ? "" : str.substr(start, end - start + 1);
}

HttpResponse::HttpResponse(const std::string& response, const std::string& clientIp, int id, std::string host) 
    :   fullResponse(response)
{
    this->id = id;
    this->clientIp = clientIp;
    this->host = host;

    std::istringstream iss(response);
    if (std::getline(iss, responseLine)) {
        responseLine = trim(responseLine);
        std::istringstream lineStream(responseLine);
        lineStream >> protocol >> status_code;
        std::getline(lineStream, reason_phrase);
        reason_phrase = trim(reason_phrase);
    }

    std::string headerLine;
    while (std::getline(iss, headerLine)) {
        headerLine = trim(headerLine);
        if (headerLine.empty())
            break;
        // "key: value"
        size_t pos = headerLine.find(":");
        if (pos != std::string::npos) {
            std::string key = trim(headerLine.substr(0, pos));
            std::string value = trim(headerLine.substr(pos + 1));
            headers[key] = value;
        }
    }

    std::ostringstream bodyStream;
    std::string bodyLine;
    while (std::getline(iss, bodyLine)) {
        bodyStream << bodyLine << "\n";
    }
    body = bodyStream.str();

    expiration = expirationTime();
}

HttpResponse::HttpResponse(int status, const std::string& clientIp, int id, std::string host) {
    this->id = id;
    this->clientIp = clientIp;
    this->host = host;
    expiration = 0;

    protocol = "HTTP/1.1";
    status_code = status;

    if (status_code == 200) {
        reason_phrase = "OK";
        body = "";
        headers["Content-Length"] = "0";
    } else if(status_code == 400){
        reason_phrase = "Bad Request";
        body = "<html><body><h1>400 Bad Request</h1></body></html>";
        headers["Content-Type"] = "text/html";
        headers["Content-Length"] = std::to_string(body.size());
    }else if(status_code == 502){
        reason_phrase = "Bad Gateway";
        body = "<html><body><h1>502 Bad Gateway</h1></body></html>";
        headers["Content-Type"] = "text/html";
        headers["Content-Length"] = std::to_string(body.size());
    }

    responseLine = protocol + " " + std::to_string(status_code) + " " + reason_phrase;

    std::ostringstream oss;
    oss << responseLine << "\r\n";
    for (const auto& header : headers) {
        oss << header.first << ": " << header.second << "\r\n";
    }
    oss << "\r\n" << body;
    fullResponse = oss.str();
}



// 104: Received "HTTP/1.1 200 OK" from www.bbc.co.uk
std::string HttpResponse::printResponse(){
    std::string answer;
    answer = std::to_string(id) + ": Received \"" +  protocol + " " + std::to_string(status_code)
         + " " + reason_phrase + "\" " + "from " + host;
    return answer;
}

void HttpResponse::httpSend(int connection_descriptor){
    size_t totalSent = 0;
    size_t toSend = fullResponse.size();
    const char* data = fullResponse.c_str();
    // std::cout << "send " << toSend << " to browser" << std::endl;
    while (totalSent < toSend) {
        ssize_t sent = send(connection_descriptor, data + totalSent, toSend - totalSent, 0);
        if (sent == -1) {
            Logger::getInstance().log(std::to_string(id) + ": ERROR send failed");
            std::cerr << "send failed" << std::endl;
            return;
        }
        totalSent += sent;
    }
}

// 104: Responding HTTP/1.1 200 OK
std::string HttpResponse::printSend(){
    std::string answer;
    answer = std::to_string(id) + ": Responding " +  protocol + " " + std::to_string(status_code)
         + " " + reason_phrase;
    return answer;
}

std::time_t HttpResponse::expirationTime(){
    // Try to get the Cache-Control header first
    auto it = headers.find("Cache-Control");
    if (it != headers.end()) {
        std::string cacheControl = it->second;
        // Look for "max-age=" directive
        std::string maxAgeDirective = "max-age=";
        size_t pos = cacheControl.find(maxAgeDirective);
        if (pos != std::string::npos) {
            pos += maxAgeDirective.length();
            size_t endPos = cacheControl.find_first_of(", ", pos);
            std::string maxAgeValue = cacheControl.substr(pos, endPos - pos);
            try {
                int maxAgeSeconds = std::stoi(maxAgeValue);
                std::time_t now = std::time(nullptr);
                std::time_t expTime = now + maxAgeSeconds;
                return expTime;
            } catch (const std::exception& e) {
                return 0;
            }
        }
    }

    // If Cache-Control doesn't specify expiration, try the Expires header.
    auto itExpires = headers.find("Expires");
    if (itExpires != headers.end()) {
        return parseTime(trim(itExpires->second));
    }
    // If neither is present, return an empty string.
    return 0;
}

std::string HttpResponse::getExpirationTime(){
    return getUtcTime(expiration);
}

std::string HttpResponse::isCachable(){
    // Check for Cache-Control header.
    auto it = headers.find("Cache-Control");
    if (it != headers.end()) {
        std::string cacheControl = it->second;
        // If "no-store" is present, it should not be cached.
        if (cacheControl.find("no-store") != std::string::npos)
            return "no-store";
        // If "private" is present, it is not cacheable by shared caches.
        if (cacheControl.find("private") != std::string::npos)
            return "private";
        // If "max-age" is present, assume it is cacheable.
        if (cacheControl.find("max-age=") != std::string::npos)
            return "";
    }
    // If there's no Cache-Control header but an Expires header exists, consider it cacheable.
    if (headers.find("Expires") != headers.end())
        return "";
    
    // Default: not cacheable because no proper caching headers were found.
    return "no cache headers";
}

bool HttpResponse::needsValidation(){
    // Check the Cache-Control header for "no-cache"
    auto it = headers.find("Cache-Control");
    if (it != headers.end()) {
        std::string cacheControl = it->second;
        if (cacheControl.find("no-cache") != std::string::npos)
            return true;
    }
    // Also check the legacy Pragma header.
    auto itPragma = headers.find("Pragma");
    if (itPragma != headers.end()) {
        std::string pragma = itPragma->second;
        if (pragma.find("no-cache") != std::string::npos)
            return true;
    }
    return false;
}

int HttpResponse::getId(){
    return id;
}

bool HttpResponse::isExpire(){
    return std::time(nullptr) > expiration;
}