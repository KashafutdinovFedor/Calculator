#pragma once

#include "plugin_api.h"
#include <windows.h>
#include <filesystem>
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace fs = std::filesystem;

class ScopedModule {
private:
    HMODULE handle_ = nullptr;

public:
    ScopedModule() = default;
    explicit ScopedModule(HMODULE h) noexcept;
    ~ScopedModule() noexcept;

    ScopedModule(const ScopedModule&) = delete;
    ScopedModule& operator=(const ScopedModule&) = delete;

    ScopedModule(ScopedModule&& other) noexcept;
    ScopedModule& operator=(ScopedModule&& other) noexcept;

    void reset(HMODULE newHandle = nullptr) noexcept;
    HMODULE get() const noexcept;
    explicit operator bool() const noexcept;
};

class PluginManager {
public:
    using UnaryMathFunc = std::function<double(double)>;

private:
    std::vector<ScopedModule> loadedModules_;
    std::map<std::string, UnaryMathFunc> functions_;

    static bool isValidIdentifier(const std::string& name);
    static bool isDllFile(const fs::directory_entry& entry);
    static const PluginDescriptor* fetchDescriptor(HMODULE module, const std::string& fileName);
    static UnaryMathFunc makeSafeWrapper(PluginExecuteFunc rawExec, const std::string& funcName);
    bool canRegister(const std::string& funcName, const std::string& fileName) const;
    bool tryLoadPlugin(const fs::path& dllPath);

public:
    PluginManager() = default;
    ~PluginManager() = default;

    PluginManager(const PluginManager&) = delete;
    PluginManager& operator=(const PluginManager&) = delete;

    void loadFromDirectory(const fs::path& pluginDir);
    double callFunction(const std::string& name, double arg) const;
    std::vector<std::string> getRegisteredFunctionNames() const;
};
