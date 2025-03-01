#include <cache.h>
#include <httpresponse.h>
#include <Logger.h>
#include <string.h>

#define maxSize 10

void Cache::addElement(HttpRequest* httprequest, HttpResponse* httpresponse){
    // delete existing item, if the item is valid, the func would not be called
    for (auto it = content.begin(); it != content.end(); ++it) {
        if(it->first->httpEqual(httprequest)){
            content.erase(it);
            break;
        }
    }

    if(content.size() == maxSize){
        content.erase(content.begin());
    }    

    std::string isCachable = httpresponse->isCachable();
    int id = httpresponse->getId();
    if(isCachable == ""){
        content.push_back(std::make_pair(httprequest, httpresponse));
        if(httpresponse->needsValidation()){
            std::cout << id << ": cached, but requires re-validation"<< std::endl;
            Logger::getInstance().log(std::to_string(id) + ": cached, but requires re-validation");
        }else{
            std::cout << id << ": cached, expires at " << httpresponse->getExpirationTime() << std::endl;
            Logger::getInstance().log(std::to_string(id) + ": cached, expires at " + httpresponse->getExpirationTime());
        }
    }else{
        std::cout << id << ": not cacheable because " << isCachable << std::endl;
        Logger::getInstance().log(
            std::to_string(id) + ": not cacheable because " + isCachable
        );
    }
}

HttpResponse* Cache::findElement(HttpRequest* httprequest){
    int id = httprequest->getId();
    for(auto& it : content){
        // std::cout << id << it.first->getUrl() << " " << httprequest->getUrl() << std::endl;
        if(it.first->httpEqual(httprequest)){
            if(it.second->needsValidation()){
                Logger::getInstance().log(
                    std::to_string(id) + ": in cache, requires validation"
                );
                std::cout << id << ": in cache, requires validation"<< std::endl;
                return nullptr;
            }else if(it.second->isExpire()){
                Logger::getInstance().log(
                    std::to_string(id) + ": in cache, but expired at " + it.second->getExpirationTime()
                );
                std::cout << id << ": in cache, but expired at " << it.second->getExpirationTime() << std::endl;
                return nullptr;
            }else{
                Logger::getInstance().log(
                    std::to_string(id) + ": in cache, valid"
                );
                std::cout << id << ": in cache, valid"<< std::endl;
                return it.second;
            }
        }
    }
    Logger::getInstance().log(
        std::to_string(id) + ": not in cache"
    );
    std::cout << id << ": not in cache"<< std::endl;
    return nullptr;
}