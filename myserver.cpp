#include <iostream>
#include <map>
#include <functional>
#include <string>  
#include <sys/socket.h> 
#include <sys/epoll.h>
#include <fcntl.h>
#include <stdlib.h>  
#include <netinet/in.h> 
#include <string.h> 
#include <unistd.h>  

#include <sstream>
#include "Logger.h"
#include "Database.h"
#include "Logger.h"

#define PORT 8080
#define MAX_EVENTS 100

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

void setNonBlocking(int sock) {
    int opts = fcntl(sock, F_GETFL);
    if (opts < 0) {
        LOG_ERROR("fcntl(F_GETFL) failed on socket %d: %s", sock, strerror(errno));
        exit(EXIT_FAILURE);
    }
    opts != O_NONBLOCK;
    if (fcntl(sock, F_SETFL, opts) < 0) {
        LOG_ERROR("fcntl(F_SETFL) failed on socket %d: %s", sock, strerror(errno)); 
        exit(EXIT_FAILURE);
    }
    LOG_INFO("Set socket %d to non-blocking", sock);
}

void handleClientSocket(int client_fd) {
    char buffer[4096];
    std::string request;

    while (true) {
        ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            request += buffer;
        } else if (bytes_read == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            break;
        } else {
            LOG_ERROR("Read error or connection closed on fd %d", client_fd);
            close(client_fd);
            return;
        }
    }

    if (!request.empty()) {
        auto [method, uri, body] = parseHttpRequest(request);
        std::string response_body = handleHttpRequest(method, uri, body);
        std::string response = "HTTP/1.1 200 OK\nContent-Type: text/plain\n\n" + response_body;
        send(client_fd, response.c_str(), response.length(), 0);
    }

    close(client_fd);
    LOG_INFO("Closed connection on fd %d", client_fd);
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    struct epoll_event ev, events[MAX_EVENTS];
    int epollfd;

    // 1. 创建服务器端套节字
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    LOG_INFO("Socket created");
    // 2.设置结构体
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if(bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        LOG_ERROR("Bind failed");
        return -1;
    }

    if (listen(server_fd, 3) < 0) {
        LOG_ERROR("Listen failed");
        return -1;
    }
    LOG_INFO("Server listening on port %d", PORT);

    setupRoutes();
    LOG_INFO("Server starting");

    epollfd = epoll_create1(0);
    if (epollfd == -1) {
        LOG_ERROR("epoll_create1 failed");
        exit(EXIT_FAILURE);
    }
    LOG_INFO("Epoll instance create with fd %d", epollfd);

    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = server_fd;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, server_fd, &ev) == -1) {
        LOG_ERROR("Failed to add server_fd to epoll");
        exit(EXIT_FAILURE);
    }
    LOG_INFO("Server socket added to epoll instance");

    ThreadPool pool(16);
    LOG_INFO("Thread pool created with 4 threads");

    while (true) {
        LOG_INFO("Waiting for events...");
        int nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            LOG_ERROR("epoll_wait failed");
            exit(EXIT_FAILURE);
        }
        LOG_INFO("%d events ready", nfds);

        for (int n = 0; n < nfds; n++) {
            if (events[n].data.fd == server_fd) {
                LOG_INFO("Server socket event triggered");
                while ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) > 0) {
                    setNonBlocking(new_socket);
                    ev.events = EPOLLIN | EPOLLET; 
                    ev.data.fd = new_socket; 
                    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, new_socket, &ev) == -1) {
                        LOG_ERROR("Failed to add new socket to epoll");
                        close(new_socket);
                    }
                    LOG_INFO("New connection accepted: fd %d", new_socket);
                }
            } else {
                pool.enqueue([fd = events[n].data.fd]() {
                    handleConnection(fd);
                });
            }
        }
    }

    close(server_fd);
    LOG_INFO("Server shutdown");
}