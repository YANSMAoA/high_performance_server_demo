
#include "HttpServer.h"
#include "Database.h"

int main(int argc, char* argv[]) {
    int port = 8080;
    if (argc > 1) {
        port = std::stoi(argv);
    }
    Database db("users.db"); // 初始化数据库
    HttpServer server(port, 10, db);
    server.setupRoutes();
    server.start();
    return 0;
}
