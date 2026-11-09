#include "plugin_manager.h"
#include <cctype>
#include <cmath>
#include <iostream>
#include <stdexcept>

ScopedModule::ScopedModule(HMODULE h) noexcept : handle_(h) {}

ScopedModule::~ScopedModule() noexcept {
    reset();
}

ScopedModule::ScopedModule(ScopedModule&& other) noexcept : handle_(other.handle_) {
    other.handle_ = nullptr;
}

ScopedModule& ScopedModule::operator=(ScopedModule&& other) noexcept {
    if (this != &other) {
        reset();
        handle_ = other.handle_;
        other.handle_ = nullptr;
    }
    return *this;
}

void ScopedModule::reset(HMODULE newHandle) noexcept {
    if (handle_) {
        FreeLibrary(handle_);
    }
    handle_ = newHandle;
}

HMODULE ScopedModule::get() const noexcept {
    return handle_;
}

ScopedModule::operator bool() const noexcept {
    return handle_ != nullptr;
}

bool PluginManager::isValidIdentifier(const std::string& name) {
    if (name.empty()) return false;
    unsigned char first = static_cast<unsigned char>(name[0]);
    if (!std::isalpha(first) && first != '_') return false;

    for (unsigned char ch : name) {
        if (!std::isalnum(ch) && ch != '_') return false;
    }
    return true;
}

namespace {
    template <typename... Parts>
    void logSkip(const Parts&... parts) {
        std::cerr << "[Skip] ";
        (std::cerr << ... << parts);
        std::cerr << "\n";
    }
}

bool PluginManager::isDllFile(const fs::directory_entry& entry) {
    std::error_code ec;
    if (!entry.is_regular_file(ec)) return false;

    std::string ext = entry.path().extension().string();
    for (char& c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return ext == ".dll";
}

const PluginDescriptor* PluginManager::fetchDescriptor(HMODULE module, const std::string& fileName) {
    auto registerFunc = reinterpret_cast<RegisterPluginFunc>(
        GetProcAddress(module, PLUGIN_ENTRY_SYMBOL)
        );

    if (!registerFunc) {
        logSkip("Библиотека '", fileName, "' не содержит точку входа '", PLUGIN_ENTRY_SYMBOL, "'.");
        return nullptr;
    }

    const PluginDescriptor* desc = nullptr;
    try {
        desc = registerFunc();
    }
    catch (...) {
        logSkip("Исключение при инициализации плагина в '", fileName, "'.");
        return nullptr;
    }

    const bool valid = desc && desc->abiVersion == PLUGIN_ABI_VERSION && desc->functionName && desc->execute;
    if (!valid) {
        logSkip("Некорректная структура дескриптора или версия ABI в '", fileName, "'.");
        return nullptr;
    }
    return desc;
}

PluginManager::UnaryMathFunc PluginManager::makeSafeWrapper(PluginExecuteFunc rawExec, const std::string& funcName) {
    return [rawExec, funcName](double arg) -> double {
        double result = 0.0;
        char errorBuf[256] = { 0 };
        int status = 0;

        try {
            status = rawExec(arg, &result, errorBuf, sizeof(errorBuf));
        }
        catch (const std::exception& e) {
            throw std::runtime_error("Исключение в функции '" + funcName + "': " + e.what());
        }
        catch (...) {
            throw std::runtime_error("Неизвестное исключение в функции '" + funcName + "'");
        }

        if (status != 0) {
            std::string errMsg = (errorBuf[0] != '\0') ? errorBuf : "ошибка вычисления";
            std::string message = "Ошибка в функции ";
            message.append(funcName).append("(").append(std::to_string(arg)).append("): ").append(errMsg);
            throw std::domain_error(message);
        }

        if (!std::isfinite(result)) {
            throw std::overflow_error("Функция " + funcName + " вернула бесконечность или NaN");
        }

        return result;
        };
}

bool PluginManager::canRegister(const std::string& funcName, const std::string& fileName) const {
    if (!isValidIdentifier(funcName)) {
        logSkip("Некорректное имя функции '", funcName, "' в плагине '", fileName, "'.");
        return false;
    }
    if (functions_.find(funcName) != functions_.end()) {
        logSkip("Конфликт имён: функция '", funcName, "' из '", fileName, "' уже зарегистрирована.");
        return false;
    }
    return true;
}

bool PluginManager::tryLoadPlugin(const fs::path& dllPath) {
    const std::string fileName = dllPath.filename().string();

    UINT oldErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);
    ScopedModule module(LoadLibraryW(dllPath.wstring().c_str()));
    const DWORD loadError = GetLastError();
    SetErrorMode(oldErrorMode);

    if (!module) {
        logSkip("Файл '", fileName, "' не является корректной DLL (код ошибки WinAPI: ", loadError, ").");
        return false;
    }

    const PluginDescriptor* desc = fetchDescriptor(module.get(), fileName);
    if (!desc) return false;

    std::string funcName(desc->functionName);
    if (!canRegister(funcName, fileName)) return false;

    functions_[funcName] = makeSafeWrapper(desc->execute, funcName);
    loadedModules_.push_back(std::move(module));

    std::cout << "[OK] Загружена функция '" << funcName
        << "' (источник: " << fileName << ")\n";
    return true;
}

void PluginManager::loadFromDirectory(const fs::path& pluginDir) {
    std::error_code ec;

    if (!fs::exists(pluginDir, ec) || !fs::is_directory(pluginDir, ec)) {
        std::cout << "[Warn] Директория '" << pluginDir.string()
            << "' не найдена. Создаём пустую директорию. "
            "Доступны только базовые операции (+, -, *, /, ^, ()).\n";
        fs::create_directories(pluginDir, ec);
        return;
    }

    std::size_t loadedCount = 0;
    std::size_t totalDlls = 0;

    for (const auto& entry : fs::directory_iterator(pluginDir, ec)) {
        if (!isDllFile(entry)) continue;

        ++totalDlls;
        if (tryLoadPlugin(entry.path())) {
            ++loadedCount;
        }
    }

    if (ec) {
        std::cerr << "[Error] Ошибка чтения директории: " << ec.message() << "\n";
    }

    if (totalDlls == 0) {
        std::cout << "[Info] В папке '" << pluginDir.string()
            << "' нет ни одного .dll файла. Доступны базовые операции.\n";
    }
    else {
        std::cout << "[Info] Загружено плагинов: " << loadedCount << " из " << totalDlls << ".\n";
    }
}

double PluginManager::callFunction(const std::string& name, double arg) const {
    auto it = functions_.find(name);
    if (it == functions_.end()) {
        throw std::invalid_argument("Неизвестная функция: '" + name + "'");
    }
    return it->second(arg);
}

std::vector<std::string> PluginManager::getRegisteredFunctionNames() const {
    std::vector<std::string> names;
    names.reserve(functions_.size());
    for (const auto& [name, _] : functions_) {
        names.push_back(name);
    }
    return names;
}
