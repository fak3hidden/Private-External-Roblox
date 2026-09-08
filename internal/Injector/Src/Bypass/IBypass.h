#pragma once
#include <string>
#include <vector>
#include <memory>
#include <windows.h>
#include "DriverProvider.h" // after interface so providers can register below

struct InjectResult {
    bool ok = false;
    std::string error;
};

// Interface every injection method implements. The injector ships with NO working
// provider: Hyperion bypasses are private by necessity (publishing = patching).
// Implement this against your own primitive (driver mapper, hijack, etc.).
class IInjectionProvider {
public:
    virtual ~IInjectionProvider() = default;
    virtual const char* Name() const = 0;
    // Check preconditions (admin rights, driver loaded, VBS state...). False + reason if unusable.
    virtual bool IsAvailable(std::string& whyNot) = 0;
    // Map/load dllPath into pid. Must leave the image executable + invoke its entry point.
    virtual InjectResult Inject(DWORD pid, const std::string& dllPath) = 0;
};

// Always-present provider that explains the situation instead of pretending.
class NullProvider final : public IInjectionProvider {
public:
    const char* Name() const override { return "null (no bypass configured)"; }
    bool IsAvailable(std::string& whyNot) override {
        whyNot = "No injection provider was compiled in. Implement IInjectionProvider (see IBypass.h + README).";
        return false;
    }
    InjectResult Inject(DWORD, const std::string&) override {
        InjectResult r;
        r.error = "No injection provider was compiled in. Implement IInjectionProvider (see IBypass.h + README).";
        return r;
    }
};

inline std::vector<std::shared_ptr<IInjectionProvider>> GetProviders() {
    // Register real providers here, e.g.: providers.push_back(std::make_shared<MyDriverProvider>());
    std::vector<std::shared_ptr<IInjectionProvider>> providers;
    providers.push_back(std::make_shared<DriverProvider>());
    providers.push_back(std::make_shared<NullProvider>());
    return providers;
}
