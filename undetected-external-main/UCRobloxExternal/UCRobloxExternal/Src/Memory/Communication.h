#pragma once
#include <windows.h>
#include <TlHelp32.h>
#include <Psapi.h>
#include <string>
#include <cstdint>
#include <memory>

class ProcessCommunicator final {
private:
    HANDLE hProcess{};
    uint32_t dwProcessId{};

    uintptr_t uModuleBase{};
    
public:
    ProcessCommunicator() = default;
    ~ProcessCommunicator() {
        if (hProcess && hProcess != INVALID_HANDLE_VALUE) {
            CloseHandle(hProcess);
        }
    }

    
    ProcessCommunicator(const ProcessCommunicator&) = delete;
    ProcessCommunicator& operator=(const ProcessCommunicator&) = delete;
    

    uint32_t FindProcessByName(const std::wstring& targetName) {
        uint32_t foundPid = 0;
        size_t maxMemory = 0;

        HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnap == INVALID_HANDLE_VALUE) return 0;
        
        PROCESSENTRY32W pe{};
        pe.dwSize = sizeof(PROCESSENTRY32W);
        
        if (Process32FirstW(hSnap, &pe)) {
            do {
                if (_wcsicmp(targetName.c_str(), pe.szExeFile) == 0) {
                    // Open process to check memory usage
                    HANDLE hTempProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe.th32ProcessID);
                    if (hTempProc) {
                        PROCESS_MEMORY_COUNTERS pmc;
                        if (GetProcessMemoryInfo(hTempProc, &pmc, sizeof(pmc))) {
                            if (pmc.WorkingSetSize > maxMemory) {
                                maxMemory = pmc.WorkingSetSize;
                                foundPid = pe.th32ProcessID;
                            }
                        }
                        CloseHandle(hTempProc);
                    } else if (foundPid == 0) {
                        // Fallback if we can't query memory info
                        foundPid = pe.th32ProcessID;
                    }
                }
            } while (Process32NextW(hSnap, &pe));
        }
        
        CloseHandle(hSnap);
        return foundPid;
    }
    



    uintptr_t FindModuleBase(const std::wstring& targetModule) {
        uintptr_t foundBase = 0;
        if (!IsConnected()) return foundBase;
        
        DWORD pid = GetProcessId(hProcess);
        HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
        if (hSnap == INVALID_HANDLE_VALUE) return foundBase;

        
        MODULEENTRY32W me{};
        me.dwSize = sizeof(MODULEENTRY32W);
        
        if (Module32FirstW(hSnap, &me)) {
            do {
                if (_wcsicmp(targetModule.c_str(), me.szModule) == 0) {
                    foundBase = reinterpret_cast<uintptr_t>(me.modBaseAddr);
                    break;
                }
            } while (Module32NextW(hSnap, &me));
        }
        
        CloseHandle(hSnap);
        return foundBase;
    }
    
    bool Connect(const std::wstring& targetName) {
        auto pid = FindProcessByName(targetName);
        if (pid == 0) return false;
        
        HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);

        if (hProc == INVALID_HANDLE_VALUE || !hProc) return false;
        
        hProcess = hProc;
        dwProcessId = pid;
        uModuleBase = FindModuleBase(targetName);
        
        return uModuleBase != 0;
    }
    
    void Disconnect() {
        if (hProcess && hProcess != INVALID_HANDLE_VALUE) {
            CloseHandle(hProcess);
            hProcess = nullptr;
        }
        dwProcessId = 0;

        uModuleBase = 0;
    }
    

    template <typename T>
    inline T ReadMemory(uintptr_t address) noexcept {
        T buffer{};
        if (!IsConnected()) [[unlikely]] return buffer;
        ReadProcessMemory(hProcess, reinterpret_cast<void*>(address), &buffer, sizeof(T), nullptr);
        return buffer;
    }
    
    template <typename T>
    inline void WriteMemory(uintptr_t address, T value) noexcept {
        if (!IsConnected()) [[unlikely]] return;

        WriteProcessMemory(hProcess, reinterpret_cast<void*>(address), &value, sizeof(T), nullptr);
    }
    
    void ReadBuffer(uintptr_t address, void* buffer, size_t size) {
        if (!IsConnected() || !buffer || size == 0) return;
        ReadProcessMemory(hProcess, reinterpret_cast<void*>(address), buffer, size, nullptr);
    }
    

    void WriteBuffer(uintptr_t address, const void* buffer, size_t size) {
        if (!IsConnected() || !buffer || size == 0) return;
        WriteProcessMemory(hProcess, reinterpret_cast<void*>(address), buffer, size, nullptr);
    }
    
    std::string ReadGameString(uintptr_t address) {
        if (!IsConnected()) return "";
        
        std::string result;
        result.reserve(64);
        

        int32_t length = ReadMemory<int32_t>(address + 0x18);
        
        if (length >= 16) {
            address = ReadMemory<uintptr_t>(address);
            if (address == 0) return "";
        }
        
        if (length > 0 && length < 4096) {
            result.resize(length);
            ReadProcessMemory(hProcess, reinterpret_cast<void*>(address), result.data(), length, nullptr);
            
            size_t nullPos = result.find('\0');

            if (nullPos != std::string::npos) {
                result.resize(nullPos);
            }
        } else {
            constexpr size_t CHUNK_SIZE = 64;
            char chunk[CHUNK_SIZE];
            size_t offset = 0;
            
            while (offset < 4096) {
                SIZE_T bytesRead = 0;
                if (!ReadProcessMemory(hProcess, reinterpret_cast<void*>(address + offset), 
                                      chunk, CHUNK_SIZE, &bytesRead) || bytesRead == 0) {
                    break;
                }
                

                for (size_t i = 0; i < bytesRead; ++i) {
                    if (chunk[i] == '\0') {
                        result.append(chunk, i);
                        return result;
                    }
                }
                
                result.append(chunk, bytesRead);
                offset += bytesRead;
                
                if (bytesRead < CHUNK_SIZE) break;
            }
        }
        
        return result;
    }
    
    void WriteGameString(uintptr_t address, const std::string& str) {
        if (!IsConnected()) return;

        int32_t length   = static_cast<int32_t>(str.length());
        int32_t capacity = ReadMemory<int32_t>(address + 0x18);

        if (length < 16) {
            // SSO (small string optimisation) — data stored inline at address
            char buf[16] = {};
            memcpy(buf, str.c_str(), length);
            WriteProcessMemory(hProcess, reinterpret_cast<void*>(address), buf, 16, nullptr);
            WriteMemory<int32_t>(address + 0x10, length);
            WriteMemory<int32_t>(address + 0x18, 15); // SSO capacity is always 15
        } else {
            // Heap path — always allocate a fresh buffer so stale pointers never cause issues
            void* remote = VirtualAllocEx(hProcess, nullptr, length + 1, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
            if (!remote) return;

            WriteProcessMemory(hProcess, remote, str.c_str(), length + 1, nullptr);
            WriteMemory<uintptr_t>(address,        reinterpret_cast<uintptr_t>(remote));
            WriteMemory<int32_t>  (address + 0x10, length);
            WriteMemory<int32_t>  (address + 0x18, length);
        }
    }


    inline bool IsConnected() const noexcept { 
        return hProcess != nullptr && hProcess != INVALID_HANDLE_VALUE; 
    }
    
    inline uint32_t GetPID() const noexcept { return dwProcessId; }
    inline uintptr_t GetBase() const noexcept { return uModuleBase; }

    inline HANDLE GetHandle() const noexcept { return hProcess; }
    
    inline void SetPID(uint32_t pid) noexcept { dwProcessId = pid; }
    inline void SetBase(uintptr_t base) noexcept { uModuleBase = base; }
};


inline std::unique_ptr<ProcessCommunicator> Coms = std::make_unique<ProcessCommunicator>();
