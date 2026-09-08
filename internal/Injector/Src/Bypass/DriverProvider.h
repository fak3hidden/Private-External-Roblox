#pragma once
#include "IBypass.h"
#include "../Drv/DriverClient.h"
#include <cstdio>

class DriverProvider final : public IInjectionProvider {
public:
    const char* Name() const override { return "kernel driver (UCRDrv)"; }

    bool IsAvailable(std::string& whyNot) override {
        UCRDriver d;
        std::string err;
        if (!d.Open(err)) {
            whyNot = err + " Is the driver loaded? (testsigning on, Memory Integrity off, sc start UCRDrv)";
            return false;
        }
        return true;
    }

    InjectResult Inject(DWORD pid, const std::string&) override {
        // STAGE 1: prove kernel R/W against the live target. The loader lands next.
        InjectResult r;
        UCRDriver d;
        std::string err;
        if (!d.Open(err)) {
            r.error = err;
            return r;
        }
        uint64_t base = 0;
        if (!d.Base(pid, base) || !base) {
            r.error = "driver R/W self-test failed: no module base";
            return r;
        }
        uint16_t mz = 0;
        if (!d.Read(pid, base, &mz, sizeof(mz)) || mz != 0x5A4D) {
            r.error = "driver R/W self-test failed: bad PE magic";
            return r;
        }
        char b[160];
        snprintf(b, sizeof(b), "driver R/W OK (base=0x%llX, MZ ok). Loader stage not implemented yet.",
            (unsigned long long)base);
        r.error = b;
        return r; // ok=false until the loader exists
    }
};
