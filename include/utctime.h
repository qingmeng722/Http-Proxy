#ifndef UTCTIME_H
#define UTCTIME_H

#include <ctime>
#include <sstream>
#include <string>
#include <iomanip>
#include <Logger.h>

//get utc time
//Sun Jan 1 22:58:17 2017
inline std::string getUtcTime() {
    std::time_t rawtime = std::time(nullptr);
    std::tm* utcTime = std::gmtime(&rawtime); 
    
    char buffer[100];
    std::strftime(buffer, sizeof(buffer), "%a %b %d %H:%M:%S %Y", utcTime);
    
    return std::string(buffer);
}

inline std::string getUtcTime(std::time_t rawtime) {
    std::tm* utcTime = std::gmtime(&rawtime); 
    
    char buffer[100];
    std::strftime(buffer, sizeof(buffer), "%a %b %d %H:%M:%S %Y", utcTime);
    
    return std::string(buffer);
}

inline std::time_t parseTime(const std::string &timeStr) {
    std::tm tm = {};
    std::istringstream ss(timeStr);
    // RFC 1123 "%a, %d %b %Y %H:%M:%S GMT"
    ss >> std::get_time(&tm, "%a, %d %b %Y %H:%M:%S GMT");
    if (ss.fail()) {
        Logger::getInstance().log("(no-id): ERROR failed to parse time");
        throw std::runtime_error("Failed to parse time: " + timeStr);
    }
    return timegm(&tm);
}

#endif // UTCTIME_H