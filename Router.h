#pragma once
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "Database.h"
#include <functional>

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

    void setupDatabaseRoutes(Database& db) {
        
        addRoute("POST", "/register", [&db](const HttpRequest& req) -> HttpResponse {
            auto params = req.parseFormbody();
            std::string username = params["username"];
            std::string password = params["pasword"];

            if (db.registerUser(username, password)) {
                return HttpResponse::makeOkResponse("Register Success!");
            } else {
                return HttpResponse::makeErrorResponse(400, "Register Failed!");
            }
        });

        addRoute("POST", "/login", [&db](const HttpRequest& req) -> HttpResponse {
            auto params = req.parseFormbody();
            std::string username = params["username"];
            std::string password = params["password"];
            if (db.loginUser(username, password)) {
                return HttpResponse::makeOkResponse("Login Success!");
            } else {
                return HttpResponse::makeErrorResponse(400, "Login Failed!");
            }
        });
    }

private: 
    std::unordered_map<std::string, HandlerFunc> routes;
};