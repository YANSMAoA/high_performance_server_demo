#include <iostream>
#include <map>
#include <functional>
#include <string>  
#include <sys/socket.h> 
#include <stdlib.h>  
#include <netinet/in.h> 
#include <string.h> 
#include <unistd.h>  

#define PORT 8080

using RequestHandler = std::function<std::string(const std::string&)>;

std::map<std::string, RequestHandler> route_table;

void setupRoutes() {
    route_table["/"] = [](const std::string& request) {
        return "HelloWorld!";
    };

    route_table["/register"] = [](const std::string& request) {
        return "RegisterSuccess!";
    };

    route_table["/login"] = [](const std::string& request) {
        return "LoginSuccess!";
    };
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // 1. 创建服务器端套节字
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    // 2.设置结构体
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));

    listen(server_fd, 3);

    setupRoutes();

    while (true) {
        new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);

        char buffer[1024] = {0};
        read(new_socket, buffer, 1024);
        std::string request(buffer);

        std::string uri = request.substr(request.find(" ") + 1);
        uri = uri.substr(0, uri.find(" "));

        std::string response_body;
        if (route_table.count(uri) > 0) {
            response_body = route_table[uri](request);
        } else {
            response_body = "404 Not Found";
        }
        
        std::string response = "HTTP/1.1 200 OK\nContent-Type: text/plain\n\n" + response_body;
        send(new_socket, response.c_str(), response.size(), 0);

        close(new_socket);
    }
}