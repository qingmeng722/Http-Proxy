#ifndef HTTPRESPONSE_H
#define HTTPRESPONSE_H

#include <iostream>
#include <sstream>
#include <string>
#include <map>
#include <algorithm>
#include <utctime.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <Logger.h>

class HttpResponse {
private:
    std::map<std::string, std::string> headers;
    std::string protocol;
    int status_code;
    std::string reason_phrase;
    std::string body;
    std::string responseLine;
    std::string fullResponse;
    std::string clientIp;
    std::string host; // server host
    std::time_t expiration;
    int id;

    std::string trim(const std::string& str);

public:
    HttpResponse(const std::string& response, const std::string& clientIp, int id, std::string host);

    HttpResponse(int status, const std::string& clientIp, int id, std::string host);

    void setExpirationTime();
    
    std::string printResponse();

    std::string printSend();

    void httpSend(int connection_descriptor);

    std::time_t expirationTime();

    std::string getExpirationTime();

    std::string isCachable();

    bool needsValidation();

    int getId();

    bool isExpire();
};

#endif // HTTPRESPONSE_H
