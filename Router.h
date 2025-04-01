#pragma once
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "Database.h"
#include <functional>
#include <fstream>

class Router {
public:
    using HandlerFunc = std::function<HttpResponse(const HttpRequest&)>;

    void addRoute(const std::string& method, const std::string& path, HandlerFunc handler) {
        routes[method + "|" + path] = handler;   
    }

    HttpResponse routeRequest(const HttpRequest& request) {
        std::string key = request.getMethodString() + "|" + request.getPath();
        if (routes.count(key)) {
            return routes[key](request);
        }

        return HttpResponse::makeErrorResponse(404, "Not Found");
    }

    std::string readFile(const std::string& filePath) {
        std::ifstream file(filePath);

        if (!file.is_open()) {
            return "Error: Unable to open file " + filePath;
        }

        std::stringstream buffer;

        buffer << file.rdbuf();

        return buffer.str();
    }

    void setupDatabaseRoutes(Database& db) {
        
        addRoute("GET", "/register", [this](const HttpRequest& req) -> HttpResponse {
            HttpResponse response;
            response.setStatusCode(200);
            response.setHeader("Content-Type", "text/html");
            response.setBody(readFile("UI/login.html"));
            return response;
        });

        addRoute("POST", "/login", [this](const HttpRequest& req) -> HttpResponse {
            HttpResponse response;
            response.setStatusCode(200);
            response.setHeader("Content-Type", "text/html");
            response.setBody(readFile("UI/register.html"));
            return response;
        });

        addRoute("POST", "/register", [&db](const HttpRequest& req) -> HttpResponse {
            auto params = req.parseFormbody();
            std::string username = params["username"];
            std::string password = params["pasword"];

            if (db.registerUser(username, password)) {
                HttpResponse response;
                response.setStatusCode(200); 
                response.setHeader("Content-Type", "text/html");
                std::string responseBody = R"(
                    <html>
                    <head>
                        <title>Register Success</title>
                        <script type="text/javascript">
                            alert("Register Success!");
                            window.location = "/login";
                        </script>
                    </head>
                    <body>
                        <h2>moving to login...</h2>
                    </body>
                    </html>
                )";
                response.setBody(responseBody);
                return response;
            } else {
                return HttpResponse::makeErrorResponse(400, "Register Failed!");
            }
        });

        addRoute("POST", "/login", [&db](const HttpRequest& req) -> HttpResponse {
            auto params = req.parseFormbody();
            std::string username = params["username"];
            std::string password = params["password"];
            if (db.loginUser(username, password)) {
                HttpResponse response;
                response.setStatusCode(200); 
                response.setHeader("Content-Type", "text/html");
                response.setBody("<html><body><h2>Login Successful</h2></body></html>");
                return response;
            } else {
                HttpResponse response;
                response.setStatusCode(401); 
                response.setHeader("Content-Type", "text/html");
                response.setBody("<html><body><h2>Login Failed</h2></body></html>");
                return response;
            }
        });
    }

private: 
    std::unordered_map<std::string, HandlerFunc> routes;
};