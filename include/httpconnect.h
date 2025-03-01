#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#include <algorithm>
#include <iostream>
#include <Logger.h>

void httpconnect(int socket1, int socket2, int id){
    const int BUF_SIZE = 10000;
    char buffer[BUF_SIZE];
    int maxfd = std::max(socket1, socket2) + 1;

    while (true) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(socket1, &readfds);
        FD_SET(socket2, &readfds);

        int activity = select(maxfd, &readfds, nullptr, nullptr, nullptr);
        if (activity < 0) {
            std::cerr << "select error" << std::endl;
            Logger::getInstance().log(std::to_string(id) + ": ERROR select error");
            break;
        }

        if (FD_ISSET(socket1, &readfds)) {
            ssize_t n = recv(socket1, buffer, BUF_SIZE, 0);
            if (n <= 0) {
                Logger::getInstance().log(std::to_string(id) + ": Tunnel closed");
                break;
            }
            ssize_t sent = send(socket2, buffer, n, 0);
            if (sent < 0) {
                std::cerr << "send error to socket2" << std::endl;
                Logger::getInstance().log(std::to_string(id) + ": ERROR send error to socket2");
                break;
            }
        }

        if (FD_ISSET(socket2, &readfds)) {
            ssize_t n = recv(socket2, buffer, BUF_SIZE, 0);
            if (n <= 0) {
                Logger::getInstance().log(std::to_string(id) + ": Tunnel closed");
                break;
            }
            ssize_t sent = send(socket1, buffer, n, 0);
            if (sent < 0) {
                std::cerr << "send error to socket1" << std::endl;
                Logger::getInstance().log(std::to_string(id) + ": ERROR send error to socket1");
                break;
            }
        }
    }
}