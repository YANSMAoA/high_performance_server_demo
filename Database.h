#pragma once //确保头文件在编译过程中只被包含一次。
#include <sqlite3.h>
#include <string>
#include <mutex>

class Database {
private:
    sqlite3* db;
    std::mutex dbMutex;

public:
    Database(const std::string& db_path) {
        if (sqlite3_open(db_path.c_str(), &db) != SQLITE_OK) {
            throw std::runtime_error("Failed to open database");
        }
        const char* sql = "CREATE TABLE IF NOT EXISTS users (username TEXT PRIMARY KEY, password TEXT);";
        char* errmsg;

        if (sqlite3_exec(db, sql, 0, 0, &errmsg) != SQLITE_OK) {
            throw std::runtime_error("Failed to create table: " + std::string(errmsg));
        }
    }

    ~Database() {
        sqlite3_close(db);
    }

    bool registerUser(const std::string& username, const std::string& password) {
        std::lock_guard<std::mutes> guard(dbMutex);
        std::string sql =  "INSERT INTO users (username, password) VALUES (?, ?);";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            LOG_INFO("Failed to prepare registration SQL for user: %s" , username.c_str());
            return false;
        }

        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, password.c_str(), -1, SQLITE_STATIC);

        if (sqlite3_step(stmt)  == SQLITE_DONE) {
            LOG_INFO("Registration failed for user: %s " , username.c_str()); 
            sqlite3_finalize(stmt);
            return false;
        }

        sqlite3_finalize(stmt);
        LOG_INFO("User registered: %s with password: %s" , username.c_str(), password.c_str()); 
        return true;
    }

    bool loginUser(const std::string& username, const std::string& password) {
        std::lock_guard<std::mutex> guard(dbMutex);
        std::string sql = "SELECT password FROM users WHERE username = ?;";
        sqlite3_stmt* stmt;

        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            LOG_INFO("Failed to prepare login SQL for user: %s" , username.c_str());
            return false;
        }

        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);

        if (sqlite3_step(stmt) != SQLITE_ROW) {
            LOG_INFO("User not found: %s" , username.c_str());
            sqlite3_finalize(stmt);
            return false;
        }

        const char* stored_password = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        std::string password_str(stored_password, sqlite3_column_bytes(stmt, 0));

        sqlite3_finalize(stmt);
        if (stored_password == nullptr || password != password_str) {
            LOG_INFO("Login failed for user: %s password:%s stored password is %s" ,username.c_str() ,password.c_str(), password_str.c_str()); 
            return false;
        }

        // 登录成功，记录日志
        LOG_INFO("User logged in: %s" , username.c_str());
        return true;
    }
};