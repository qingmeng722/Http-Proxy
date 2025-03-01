#include <httprequest.h>
#include <httpresponse.h>
#include <vector>

class Cache{
private:
    std::vector<std::pair<HttpRequest*, HttpResponse*>> content;

public:
    Cache(){}

    void addElement(HttpRequest* httprequest, HttpResponse* httpresponse);

    HttpResponse* findElement(HttpRequest* httprequest);
};