#pragma once

#include <string>
#include <unordered_map>
#include <sstream>

class HttpRequest {
public: 
    enum Method {
        GET, POST, HEAD, PUT, DELETE, TRACE, OPTIONS, CONNECT, PATCH, UNKOWN
    };

    enum ParseState {
        REQUEST_LINE, HEADERS, BODY, FINISH
    };

    HttpRequest() : method(UNKOWN), state(REQUEST_LINE) {}

    /*
    POST /login HTTP/1.1 // 行

    Host: localhost:7007
    Content-Type: application/x-www-form-urlencoded
    Content-Length: 30 // 头
    
    username=yuanshen&password=test1 // 体
    */

    bool parse(std::string request) {
        std::istringstream iss(request);
        std::string line;
        bool result = true;

        while (std::getline(iss, line) && line != "\r") {
            if (state == REQUEST_LINE) {
                result = parseRequestLine(line);
            } else if (state == HEADERS) {
                result = parseHeader(line);
            }
            if (!result) {
                break;
            }
        }

        if (method == POST) {
            body = request.substr(request.find("\r\n\r\n") + 4);
        }

        return result;
    }

    std::unordered_map<std::string, std::string> parseFormbody() const {
        std::unordered_map<std::string, std::string> params;
        if (method != POST) return params;

        std::istringstream stream(body);
        std::string pair;

        while (std::getline(stream, pair, '&')) {
            std::size_t pos = pair.find('=');
            if (pos == std::string::npos) continue;
            std::string key = pair.substr(0, pos);
            std::string value = pair.substr(pos + 1);
            params[key] = value;
        }

        return params;
    }

    std::string getMethodString() const {
        switch (method) {
            case GET: return "GET";
            case POST: return "POST";
            //...
            default: return "UNKNOWN";
        }
    }

    std::string getPath() const {
        return path;
    }

private: 
    bool parseRequestLine(const std::string& line) {
        std::istringstream iss(line);
        std::string method_str;
        iss >> method_str;
        if (method_str == "GET") method = GET;
        else if (method_str == "POST") method = POST;
        else method = UNKOWN;

        iss >> path;
        iss >> version;
        state = HEADERS;
        return true;
    }

    bool parseHeader(const std::string& line) {
        size_t pos = line.find(": ");
        if (pos == std::string::npos) {
            return false;
        }
        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 2);
        headers[key] = value;
        return true;
    }

    Method method;
    std::string path;
    std::string version;
    std::unordered_map<std::string, std::string> headers;
    ParseState state;
    std::string body;
};