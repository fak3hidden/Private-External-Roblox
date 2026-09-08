#pragma once
#include <string>
#include <fstream>
#include <mutex>
#include <cstdio>
#include <cstdint>

class FileLog {
public:
    static FileLog& Instance() {
        static FileLog inst;
        return inst;
    }
    void Init(const std::string& path) {
        std::lock_guard<std::mutex> l(mtx_);
        path_ = path;
        std::ofstream f(path_, std::ios::trunc);
        if (f) f << "--- log start ---\n";
    }
    void Write(const std::string& msg) {
        std::lock_guard<std::mutex> l(mtx_);
        if (path_.empty()) return;
        std::ofstream f(path_, std::ios::app);
        if (f) f << msg << "\n";
    }
    void WriteHex(const char* label, uintptr_t v) {
        char b[96];
        snprintf(b, sizeof(b), "%s: 0x%llX", label, (unsigned long long)v);
        Write(b);
    }
private:
    FileLog() = default;
    std::string path_;
    std::mutex mtx_;
};
