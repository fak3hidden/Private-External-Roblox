#pragma once
// Runtime offset auto-updater for the external.
//
// Fetches theo's live dump (https://offsets.imtheo.lol/offsets.json), parses the
// flat "Namespace::Field" table, and applies it into the runtime-mutable
// Offsets::* variables (see OffsetsRegistry.h) BEFORE the first pointer scan.
//
// Safety rule: offsets are only ever applied when the dump's Roblox version
// EXACTLY matches the version of the process we are attached to. Writing
// offsets from build A into build B corrupts memory, so on any mismatch (or
// fetch/parse failure) we keep the baked-in values and say so.
//
// WinHTTP only -- no third-party dependencies.
#include <windows.h>
#include <winhttp.h>
#include <string>
#include <map>
#include <vector>
#include <cstdint>
#include <cstdio>
#include "Offsets.h"
#include "OffsetsRegistry.h"

#pragma comment(lib, "winhttp.lib")

namespace Offsets {

    inline const wchar_t* kOffsetUrl = L"https://offsets.imtheo.lol/offsets.json";

    // --- HTTP GET (WinHTTP) -------------------------------------------------
    inline bool HttpGet(const wchar_t* url, std::string& out, std::string& err) {
        out.clear();
        HINTERNET hS = WinHttpOpen(L"UCR-Offsets/1.0",
            WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
        if (!hS) { err = "WinHttpOpen failed"; return false; }

        URL_COMPONENTS uc{};
        uc.dwStructSize = sizeof(uc);
        wchar_t host[256] = {}, path[2048] = {};
        uc.lpszHostName = host;  uc.dwHostNameLength = 256;
        uc.lpszUrlPath = path;   uc.dwUrlPathLength = 2048;

        if (!WinHttpCrackUrl(url, (DWORD)wcslen(url), 0, &uc)) {
            err = "WinHttpCrackUrl failed";
            WinHttpCloseHandle(hS);
            return false;
        }

        HINTERNET hC = WinHttpConnect(hS, host, uc.nPort, 0);
        if (!hC) { err = "WinHttpConnect failed"; WinHttpCloseHandle(hS); return false; }

        DWORD flags = (uc.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
        HINTERNET hR = WinHttpOpenRequest(hC, L"GET", (path[0] ? path : nullptr),
            nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
        if (!hR) { err = "WinHttpOpenRequest failed"; WinHttpCloseHandle(hC); WinHttpCloseHandle(hS); return false; }

        bool ok = false;
        if (!WinHttpSendRequest(hR, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
            err = "WinHttpSendRequest failed";
        }
        else if (!WinHttpReceiveResponse(hR, nullptr)) {
            err = "WinHttpReceiveResponse failed";
        }
        else {
            DWORD avail = 0;
            for (;;) {
                if (!WinHttpQueryDataAvailable(hR, &avail)) { err = "QueryDataAvailable failed"; break; }
                if (avail == 0) break;
                if (avail > (1u << 20)) avail = (1u << 20); // safety clamp
                std::vector<char> buf(avail);
                DWORD read = 0;
                if (!WinHttpReadData(hR, buf.data(), avail, &read)) { err = "ReadData failed"; break; }
                if (read == 0) break;
                out.append(buf.data(), read);
            }
            if (out.empty()) err = "empty response";
            else ok = true;
        }

        WinHttpCloseHandle(hR);
        WinHttpCloseHandle(hC);
        WinHttpCloseHandle(hS);
        return ok;
    }

    // --- Minimal JSON parser (tuned to offsets.json schema) -----------------
    // Top: { "Roblox Version": "...", "Total Offsets": N, "Offsets": { Ns: {Field: int} } }
    struct OffsetJson {
        const char* p = nullptr;
        const char* end = nullptr;
        std::map<std::string, uintptr_t> offsets; // "Ns::Field" -> value
        std::string version;
        int total = 0;

        void ws() {
            while (p < end && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) ++p;
        }
        bool ch(char c) {
            ws();
            if (p < end && *p == c) { ++p; return true; }
            return false;
        }
        bool str(std::string& o) {
            ws();
            if (p >= end || *p != '"') return false;
            ++p; o.clear();
            while (p < end && *p != '"') {
                if (*p == '\\') {
                    ++p;
                    if (p >= end) return false;
                    char e = *p++;
                    switch (e) {
                        case '"':  o += '"';  break;
                        case '\\': o += '\\'; break;
                        case '/':  o += '/';  break;
                        case 'n':  o += '\n'; break;
                        case 't':  o += '\t'; break;
                        case 'r':  o += '\r'; break;
                        case 'u':  if (p + 4 > end) return false; p += 4; o += '?'; break;
                        default:   o += e; break;
                    }
                }
                else o += *p++;
            }
            if (p < end && *p == '"') { ++p; return true; }
            return false;
        }
        bool num(uintptr_t& o) {
            ws();
            if (p >= end) return false;
            bool neg = false;
            if (*p == '-') { neg = true; ++p; }
            if (p >= end || !isdigit((unsigned char)*p)) return false;
            unsigned long long v = 0;
            while (p < end && isdigit((unsigned char)*p)) { v = v * 10 + (*p - '0'); ++p; }
            o = neg ? (uintptr_t)(0ULL - v) : (uintptr_t)v;
            return true;
        }
        bool skipObject() { // { ... } including nested values
            ws();
            if (!ch('{')) return false;
            for (;;) {
                ws();
                if (p < end && *p == '}') { ++p; return true; }
                std::string k;
                if (!str(k)) return false;
                if (!ch(':')) return false;
                if (!skipValue()) return false;
                ws();
                if (p < end && *p == ',') { ++p; continue; }
                if (p < end && *p == '}') { ++p; return true; }
                return false;
            }
        }
        bool skipValue() {
            ws();
            if (p >= end) return false;
            char c = *p;
            if (c == '"') { std::string t; return str(t); }
            if (c == '{') { return skipObject(); }
            if (c == '[') {
                ++p;
                for (;;) {
                    ws();
                    if (p < end && *p == ']') { ++p; return true; }
                    if (!skipValue()) return false;
                    ws();
                    if (p < end && *p == ',') ++p;
                }
            }
            if (c == 't') { if (p + 4 > end) return false; p += 4; return true; }
            if (c == 'f') { if (p + 5 > end) return false; p += 5; return true; }
            if (c == 'n') { if (p + 4 > end) return false; p += 4; return true; }
            uintptr_t v; return num(v);
        }
        bool fields(const std::string& ns) {
            ws();
            if (!ch('{')) return false;
            for (;;) {
                ws();
                if (p < end && *p == '}') { ++p; return true; }
                std::string f;
                if (!str(f)) return false;
                if (!ch(':')) return false;
                ws();
                if (p >= end) return false;
                if (*p == '"' || *p == '{' || *p == '[') { if (!skipValue()) return false; }
                else if (*p == '-' || isdigit((unsigned char)*p)) {
                    uintptr_t v;
                    if (!num(v)) return false;
                    offsets[ns + "::" + f] = v;
                }
                else { if (!skipValue()) return false; } // true/false/null
                ws();
                if (p < end && *p == ',') { ++p; continue; }
                if (p < end && *p == '}') { ++p; return true; }
                return false;
            }
        }
        bool namespaces() {
            ws();
            if (!ch('{')) return false;
            for (;;) {
                ws();
                if (p < end && *p == '}') { ++p; return true; }
                std::string ns;
                if (!str(ns)) return false;
                if (!ch(':')) return false;
                ws();
                if (p >= end) return false;
                if (*p == '{') { if (!fields(ns)) return false; }
                else if (!skipValue()) return false;
                ws();
                if (p < end && *p == ',') { ++p; continue; }
                if (p < end && *p == '}') { ++p; return true; }
                return false;
            }
        }
        bool parse() {
            ws();
            if (!ch('{')) return false;
            for (;;) {
                ws();
                if (p < end && *p == '}') { ++p; return true; }
                std::string k;
                if (!str(k)) return false;
                if (!ch(':')) return false;
                ws();
                if (p >= end) return false;
                if (*p == '"') {
                    std::string v;
                    if (!str(v)) return false;
                    if (k == "Roblox Version") version = v;
                }
                else if (*p == '{') {
                    if (k == "Offsets") { if (!namespaces()) return false; }
                    else if (!skipValue()) return false;
                }
                else if (*p == '-' || isdigit((unsigned char)*p)) {
                    uintptr_t v;
                    if (!num(v)) return false;
                    if (k == "Total Offsets") total = (int)v;
                }
                else { if (!skipValue()) return false; }
                ws();
                if (p < end && *p == ',') { ++p; continue; }
                if (p < end && *p == '}') { ++p; return true; }
                return false;
            }
        }
    };

    // --- Apply a parsed table into the mutable Offsets::* variables ---------
    inline int ApplyOffsets(const std::map<std::string, uintptr_t>& map) {
        int applied = 0;
        for (size_t i = 0; i < g_RegistryCount; ++i) {
            auto it = map.find(g_Registry[i].name);
            if (it != map.end()) { *g_Registry[i].var = it->second; ++applied; }
        }
        return applied;
    }

    // --- Entry point: call once after attaching, before the first scan ------
    // Returns true if fresh offsets were applied for the running build.
    inline bool AutoUpdate(HANDLE hProc) {
        // Detect the running build from the image folder name (same heuristic
        // the version gate uses: ...\versions\version-xxxxxxxx\RobloxPlayerBeta.exe).
        std::string running;
        {
            wchar_t path[MAX_PATH];
            DWORD sz = MAX_PATH;
            if (QueryFullProcessImageNameW(hProc, 0, path, &sz)) {
                std::wstring p(path);
                size_t e = p.find_last_of(L"\\/");
                if (e != std::wstring::npos) {
                    size_t s = p.find_last_of(L"\\/", e - 1);
                    std::wstring folder = (s == std::wstring::npos) ? p.substr(0, e)
                                                                    : p.substr(s + 1, e - s - 1);
                    running.assign(folder.begin(), folder.end());
                }
            }
        }

        std::string json, err;
        if (!HttpGet(kOffsetUrl, json, err)) {
            std::printf("[!] Offsets: fetch failed (%s) -- using baked offsets.\n", err.c_str());
            return false;
        }
        OffsetJson ps;
        ps.p = json.c_str();
        ps.end = ps.p + json.size();
        if (!ps.parse()) {
            std::printf("[!] Offsets: JSON parse failed -- using baked offsets.\n");
            return false;
        }
        std::printf("[*] Offsets: fetched %d offsets for %s\n", ps.total, ps.version.c_str());

        if (running.empty()) {
            std::printf("[!] Offsets: could not detect running version -- keeping baked (%s).\n",
                ClientVersion.c_str());
            return false;
        }
        if (ps.version != running) {
            std::printf("[!] Offsets: running %s but latest dump is %s -- keeping baked (%s).\n",
                running.c_str(), ps.version.c_str(), ClientVersion.c_str());
            return false;
        }
        int n = ApplyOffsets(ps.offsets);
        ClientVersion = ps.version;
        std::printf("[+] Offsets: applied %d/%d entries for %s.\n",
            n, (int)ps.offsets.size(), ps.version.c_str());
        return n > 0;
    }

} // namespace Offsets
