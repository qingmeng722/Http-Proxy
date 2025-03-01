#ifndef LOGGER_H
#define LOGGER_H

#include <fstream>
#include <string>
#include <mutex>

class Logger {
public:
    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }

    void log(const std::string& message) {
        std::lock_guard<std::mutex> lock(logMutex);
        logFile << message << std::endl;
    }

private:
    std::ofstream logFile;
    std::mutex logMutex;

    Logger() {
        logFile.open("/var/log/erss/proxy.log", std::ios_base::app);
        // logFile.open("proxy.log", std::ios_base::app);
        if (!logFile.is_open()) {
            throw std::runtime_error("Unable to open log file.");
        }
    }

    ~Logger() {
        if (logFile.is_open()) {
            logFile.close();
        }
    }

    // Prevent copying and assignment
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
};

#endif // LOGGER_H
