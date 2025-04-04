#ifndef DATABASE_H
#define DATABASE_H

#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/json.hpp>
#include <string>
#include <future>
#include <iostream> // 添加标准输出库

class Database {
private:
    mongocxx::instance instance{}; // 全局实例
    mongocxx::client client;       // MongoDB 客户端
    mongocxx::database db;         // 数据库

public:
    // 构造函数
    Database(const std::string& uri) {
        LOG_INFO("Connecting to MongoDB");
        std::cout << "Connecting to MongoDB at: " << uri << std::endl;
        client = mongocxx::client{mongocxx::uri{uri}};
        db = client["userdb"]; // 使用 userdb 数据库
    }

    // 异步注册用户
    std::future<bool> registerUserAsync(const std::string& username, const std::string& password) {
        return std::async(std::launch::async, [this, username, password]() {
            return this->registerUser(username, password);
        });
    }

    // 异步登录用户
    std::future<bool> loginUserAsync(const std::string& username, const std::string& password) {
        return std::async(std::launch::async, [this, username, password]() {
            return this->loginUser(username, password);
        });
    }

    // 注册用户
    bool registerUser(const std::string& username, const std::string& password) {
        std::cout << "registerUser begin" << std::endl; // 调试信息
        LOG_INFO("User Register");
        bsoncxx::builder::stream::document document{};
        document << "username" << username << "password" << password;

        auto collection = db["users"];
        bsoncxx::stdx::optional<mongocxx::result::insert_one> result = collection.insert_one(document.view());

        return result ? true : false;
    }

    // 登录用户
    bool loginUser(const std::string& username, const std::string& password) {
        LOG_INFO("User Login");
        auto collection = db["users"];
        bsoncxx::builder::stream::document document{};
        document << "username" << username;

        auto cursor = collection.find(document.view());
        for (auto&& doc : cursor) {
            std::string stored_password = doc["password"].get_utf8().value.to_string();
            if (password == stored_password) {
                return true;
            }
        }
        return false;
    }

     // 存储图片信息
    bool storeImage(const std::string& imageName, const std::string& imagePath, const std::string& description) {
        bsoncxx::builder::stream::document document{};
        document << "name" << imageName
                 << "path" << imagePath
                 << "description" << description;

        auto collection = db["images"];
        bsoncxx::stdx::optional<mongocxx::result::insert_one> result = collection.insert_one(document.view());
        return result ? true : false;
    }

    // 获取图片列表
    std::vector<std::string> getImageList() {
        std::vector<std::string> images;
        auto collection = db["images"];
        auto cursor = collection.find({});
        for (auto&& doc : cursor) {
            images.push_back(doc["path"].get_utf8().value.to_string());
        }
        return images;
    }
};

#endif // DATABASE_H





// #pragma once //确保头文件在编译过程中只被包含一次。
// #include <sqlite3.h>
// #include <string>
// #include <mutex>

// class Database {
// private:
//     sqlite3* db;
//     std::mutex dbMutex;

// public:
//     Database(const std::string& db_path) {
//         if (sqlite3_open(db_path.c_str(), &db) != SQLITE_OK) {
//             throw std::runtime_error("Failed to open database");
//         }
//         const char* sql = "CREATE TABLE IF NOT EXISTS users (username TEXT PRIMARY KEY, password TEXT);";
//         char* errmsg;

//         if (sqlite3_exec(db, sql, 0, 0, &errmsg) != SQLITE_OK) {
//             throw std::runtime_error("Failed to create table: " + std::string(errmsg));
//         }
//     }

//     ~Database() {
//         sqlite3_close(db);
//     }

//     bool registerUser(const std::string& username, const std::string& password) {
//         std::lock_guard<std::mutes> guard(dbMutex);
//         std::string sql =  "INSERT INTO users (username, password) VALUES (?, ?);";
//         sqlite3_stmt* stmt;
//         if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
//             LOG_INFO("Failed to prepare registration SQL for user: %s" , username.c_str());
//             return false;
//         }

//         sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
//         sqlite3_bind_text(stmt, 2, password.c_str(), -1, SQLITE_STATIC);

//         if (sqlite3_step(stmt)  == SQLITE_DONE) {
//             LOG_INFO("Registration failed for user: %s " , username.c_str()); 
//             sqlite3_finalize(stmt);
//             return false;
//         }

//         sqlite3_finalize(stmt);
//         LOG_INFO("User registered: %s with password: %s" , username.c_str(), password.c_str()); 
//         return true;
//     }

//     bool loginUser(const std::string& username, const std::string& password) {
//         std::lock_guard<std::mutex> guard(dbMutex);
//         std::string sql = "SELECT password FROM users WHERE username = ?;";
//         sqlite3_stmt* stmt;

//         if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
//             LOG_INFO("Failed to prepare login SQL for user: %s" , username.c_str());
//             return false;
//         }

//         sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);

//         if (sqlite3_step(stmt) != SQLITE_ROW) {
//             LOG_INFO("User not found: %s" , username.c_str());
//             sqlite3_finalize(stmt);
//             return false;
//         }

//         const char* stored_password = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
//         std::string password_str(stored_password, sqlite3_column_bytes(stmt, 0));

//         sqlite3_finalize(stmt);
//         if (stored_password == nullptr || password != password_str) {
//             LOG_INFO("Login failed for user: %s password:%s stored password is %s" ,username.c_str() ,password.c_str(), password_str.c_str()); 
//             return false;
//         }

//         // 登录成功，记录日志
//         LOG_INFO("User logged in: %s" , username.c_str());
//         return true;
//     }
// };