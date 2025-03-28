#include <fstream>
#include <string>
#include <chrono>
#include <ctime>
#include <cstdarg>

enum LogLevel {
    INFO,
    WARNING,
    ERROR
};

class Logger {
public:
    static void LogMessage(LogLevel level, const char* foramt, ...) {
        std::ofstream LogFile("server.log", std::ios::app); 

        auto now = std::chrono::system_clock::now();
        auto now_c = std::chrono::system_clock::to_time_t(now);

        std::string levelStr;
        switch (level) {
            case INFO : levelStr = "INFO"; break;
            case WARNING : levelStr = "WARNING"; break;
            case ERROR : levelStr = "ERROR"; break;
        }

        va_list args;
        va_start(args, foramt);
        char buffer[2048];
        vsnprintf(buffer, sizeof(buffer), foramt, args);

        va_end(args);

        LogFile << std::ctime(&now_c) << " [" << levelStr << "]" << buffer << std::endl;

        LogFile.close();
    }
};

//LOG_INFO("Hello, %s", name) => Logger::LogMessage(INFO, "Hello, %s", name)。

#define LOG_INFO(...) Logger::LogMessage(INFO, __VA_ARGS__)
#define LOG_WARNING(...) Logger::LogMessage(WARNING, __VA_ARGS__)
#define LOG_ERROR(...) Logger::LogMessage(ERROR, __VA_ARGS__)