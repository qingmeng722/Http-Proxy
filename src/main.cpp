#include <arpa/inet.h>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <cstdlib>
#include <iostream>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <httprequest.h>
#include <httpresponse.h>
#include <httpconnect.h>
#include <cache.h>
#include <Logger.h> // for log
#include <string>

// max listen number
#define BACKLOG 10
const int BUFFER_SIZE = 8196;

namespace beast = boost::beast;
namespace http = beast::http;

void server(int id, int connection_descriptor, Cache* cache) {
  // get client_ip
  struct sockaddr_in client_addr;
  socklen_t client_addr_len = sizeof(client_addr);
  char client_ip[INET_ADDRSTRLEN] = "UNKNOWN";
  std::string hostnumber;
  if (getpeername(connection_descriptor, (struct sockaddr *)&client_addr, &client_addr_len) == -1) {
    Logger::getInstance().log(std::to_string(id) + ": ERROR getpeername() failed");
    std::cerr << "getpeername failed" << std::endl;
  } else {
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
    hostnumber = std::to_string(ntohs(client_addr.sin_port));
  }

  // Create a Beast flat_buffer for accumulating incoming data.
  beast::flat_buffer b;

  // Create an HTTP request parser.
  http::request_parser<http::string_body> parser;
  beast::error_code ec;

  // Enable eager parsing.
  parser.eager(true);

  // Loop to receive data and parse the HTTP request incrementally.
  while (!parser.is_done()) {
      char buffer[BUFFER_SIZE] = {0};
      // Receive data from the client.
      ssize_t bytes_received = recv(connection_descriptor, buffer, BUFFER_SIZE, 0);
      if (bytes_received <= 0) {
          // std::cerr << "Receive failed or connection closed" << std::endl;
          close(connection_descriptor);
          return;
      }

      // Append the received data into the flat_buffer.
      std::size_t data_size = static_cast<std::size_t>(bytes_received);
      auto buffer_space = b.prepare(data_size);
      std::size_t bytes_copied = boost::asio::buffer_copy(buffer_space, boost::asio::buffer(buffer, data_size));
      b.commit(bytes_copied);

      // Attempt to parse the HTTP request with the accumulated data.
      parser.put(b.data(), ec);
      // If an error occurs and it is not simply a "need more data" error, handle it.
      if (ec && ec != beast::http::error::need_more) {
          std::cerr << "HTTP request parsing failed: " << ec.message() << std::endl;
          Logger::getInstance().log(std::to_string(id) + ": ERROR HTTP request parsing failed: "+ec.message());
          auto httpresponse = new HttpResponse(400, client_ip, id, hostnumber);
          httpresponse->httpSend(connection_descriptor);
          close(connection_descriptor);
          return;
      }
  }

  // Retrieve the parsed HTTP request.
  auto httprequest = new HttpRequest(parser.get(), client_ip, id);
  Logger::getInstance().log(
    httprequest->printRequest()
  );
  std::cout << httprequest->printRequest() << std::endl;

  if(httprequest->getMethod() == "GET"){
    auto httpresponse = cache->findElement(httprequest);
    if(httpresponse != nullptr){
      httpresponse->httpSend(connection_descriptor);
      Logger::getInstance().log(
        httpresponse->printSend()
      );
      std::cout << httpresponse->printSend() << std::endl;
    }else{
      std::string response = httprequest->httpGetPost();
      if(response == "-1"){
        auto httpresponse = new HttpResponse(502, client_ip, id, httprequest->getHost());
        httpresponse->httpSend(connection_descriptor);
        Logger::getInstance().log(std::to_string(id) + ": WARNING Http GetPost Failed");
        close(connection_descriptor);
        return;
      }
      httpresponse = new HttpResponse(response, client_ip, id, httprequest->getHost());
      Logger::getInstance().log(
        httpresponse->printResponse()
      );
      std::cout << httpresponse->printResponse() << std::endl;
      httpresponse->httpSend(connection_descriptor);
      Logger::getInstance().log(
        httpresponse->printSend()
      );
      std::cout << httpresponse->printSend() << std::endl;
      cache->addElement(httprequest, httpresponse);
    }
  }else if(httprequest->getMethod() == "POST"){
    std::string response = httprequest->httpGetPost();
    if(response == "-1"){
      auto httpresponse = new HttpResponse(502, client_ip, id, httprequest->getHost());
      httpresponse->httpSend(connection_descriptor);
      Logger::getInstance().log(std::to_string(id) + ": WARNING Http GetPost Failed");
      close(connection_descriptor);
      return;
    }
    auto httpresponse = new HttpResponse(response, client_ip, id, httprequest->getHost());

    Logger::getInstance().log(
      httpresponse->printResponse()
    );
    std::cout << httpresponse->printResponse() << std::endl;

    httpresponse->httpSend(connection_descriptor);

    Logger::getInstance().log(
      httpresponse->printSend()
    );
    std::cout << httpresponse->printSend() << std::endl;
  }else if(httprequest->getMethod() == "CONNECT"){
    int socket_fd = httprequest->httpConnect();
    if(socket_fd == -1){
      auto httpresponse = new HttpResponse(502, client_ip, id, httprequest->getHost());
      httpresponse->httpSend(connection_descriptor);
      Logger::getInstance().log(std::to_string(id) + ": WARNING Http Connect Failed");
      close(connection_descriptor);
      return;
    }
    auto httpresponse = new HttpResponse(200, client_ip, id, httprequest->getHost());
    // std::cout << httpresponse->printResponse() << std::endl;
    httpresponse->httpSend(connection_descriptor);

    Logger::getInstance().log(
      httpresponse->printSend()
    );
    std::cout << httpresponse->printSend() << std::endl;
    httpconnect(connection_descriptor, socket_fd, id);
    close(socket_fd);
  }else{
    Logger::getInstance().log(std::to_string(id) + ": ERROR " + httprequest->getMethod() + " not support");
    std::cerr << httprequest->getMethod() << " not support"<< std::endl;
  }

  close(connection_descriptor);
}

int main(int argc, char *argv[]) {
  if (argc > 2) {
    Logger::getInstance().log("(no-id): ERROR too much arguments");
    std::cerr << "too much arguments" << std::endl;
    return EXIT_FAILURE;
  }

  int port_num = 12345; // default port number
  if (argc == 2) {
    port_num = std::stoi(argv[1]);
  }

  int server_fd;
  struct sockaddr_in server_addr;

  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd == -1) {
    Logger::getInstance().log("(no-id): ERROR socket creation fail");
    std::cerr << "socket creation fail" << std::endl;
    return EXIT_FAILURE;
  }

  int opt = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
      Logger::getInstance().log("(no-id): ERROR setsockopt failed");
      std::cerr << "setsockopt failed" << std::endl;
      close(server_fd);
      return EXIT_FAILURE;
  }

  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(port_num);

  if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) ==
      -1) {
    Logger::getInstance().log("(no-id): ERROR bind fail");
    std::cerr << "bind fail" << std::endl;
    close(server_fd);
    return EXIT_FAILURE;
  }

  if (listen(server_fd, BACKLOG) == -1) {
    Logger::getInstance().log("(no-id): ERROR listen fail");
    std::cerr << "listen fail" << std::endl;
    close(server_fd);
    return EXIT_FAILURE;
  }

  auto cache = new Cache();
  int id = 1; // used for client info
  while (true) {
    auto addrlen = sizeof(server_addr);
    int connection_descriptor = accept(
        server_fd, (struct sockaddr *)&server_addr, (socklen_t *)&addrlen);
    if (connection_descriptor < 0) {
      Logger::getInstance().log("(no-id): ERROR handle connection with client fail");
      std::cerr << "handle connection with client fail" << std::endl;
      return EXIT_FAILURE;
    }

    std::thread t(server, id, connection_descriptor, cache);
    t.detach();
    id++;
  }
  close(server_fd);
  return 0;
}