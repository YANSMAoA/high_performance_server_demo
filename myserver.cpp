#include <iostream>
#include <map>
#include <functional>
#include <string>  
#include <sys/socket.h> 
#include <stdlib.h>  
#include <netinet/in.h> 
#include <string.h> 
#include <unistd.h>  

#include <sstream>
#include "Logger.h"
#include "Database.h"

#define PORT 8080

using RequestHandler = std::function<std::string(const std::string&)>;

std::map<std::string, RequestHandler> get_routes;
std::map<std::string, RequestHandler> post_routes;
Database db("users.db");

std::map<std::string, std::string> parseFormBody(const std::string& body) {
    std::map<std::string, std::string> params;
    std::istringstream steam(body);
    std::string pair;

    LOG_INFO("Parsing body: %s", body.c_str());  

    while (std::getline(steam, pair, '&')) {
        std::string::size_type pos = pair.find('=');
        if (pos != std::string::npos) {
            std::string key = pair.substr(0, pos);
            std::string value = pair.substr(pos + 1);
            params[key] = value;

            LOG_INFO("Parsed key-value pair: %s = %s" , key.c_str(), value.c_str());
        } else {
            std::string error_msg = "Error parsing: " + pair;
            LOG_ERROR(error_msg.c_str()); 
            std::cerr << error_msg << std::endl;
        }
    }
    return params;
}

void setupRoutes() {
    LOG_INFO("Setting up routes");
    get_routes["/"] = [](const std::string& request) {
        return "HelloWorld!";
    };

    get_routes["/register"] = [](const std::string& request) {
        return "Please use POST to register";
    };

    get_routes["/login"] = [](const std::string& request) {
        return "Please use POST to login";
    };

    post_routes["/register"] = [](const std::string& request) {
        auto params = parseFormBody(request);
        std::string username = params["username"];
        std::string password = params["password"];

        if (db.registerUser(username, password)) {
            return "Register Success!";
        } else {
            return "Register Failed!";
        }
    };

    post_routes["/login"] = [](const std::string& request) {
        
        auto params = parseFormBody(request);
        std::string username = params["username"];
        std::string password = params["password"];

        if (db.loginUser(username, password)) {
            return "Login Success!";
        } else {
            return "Login Failed!";
        }
    };
}

std::tuple<std::string, std::string, std::string> parseHttpRequest(const std::string& request) {
    LOG_INFO("Prasing HTTP requset");

    size_t method_end = request.find(" ");
    std::string method = request.substr(0, method_end);

    size_t uri_end = request.find(" ", method_end + 1);
    std::string uri = request.substr(method_end + 1, uri_end - method_end - 1);

    std::string body;
    if (method == "POST") {
        size_t body_start = request.find("\r\n\r\n");
        if (body_start != std::string::npos) {
            body = request.substr(body_start + 4);
        }
    }

    return {method, uri, body};
}

std::string handleHttpRequest(const std::string& method, const std::string& uri, const std::string& body) {
    LOG_INFO("Handing HTTP request for URI : %s", uri.c_str());
    if (method == "GET" && get_routes.count(uri) > 0) {
        return get_routes[uri](body);
    }
    else if (method == "POST" && post_routes.count(uri) > 0) {
        return post_routes[uri](body);
    } else {
        return "404 Not Found";
    }
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // 1. 创建服务器端套节字
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    LOG_INFO("Socket created");
    // 2.设置结构体
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));

    listen(server_fd, 3);
    LOG_INFO("Server listening on port %d", PORT);

    setupRoutes();
    LOG_INFO("Server starting");

    while (true) {
        new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);

        char buffer[1024] = {0};
        read(new_socket, buffer, 1024);
        std::string request(buffer);

        auto [method, uri, body] = parseHttpRequest(request);

        std::string response_body = handleHttpRequest(method, uri, request);
        
        std::string response = "HTTP/1.1 200 OK\nContent-Type: text/plain\n\n" + response_body;
        send(new_socket, response.c_str(), response.size(), 0);

        close(new_socket);
    }
}