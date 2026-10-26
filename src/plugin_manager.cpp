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
        if (ec) {
            std::cerr << "[Error] Ошибка чтения элемента директории: " << ec.message() << "\n";
            continue;
        }

        if (!entry.is_regular_file(ec)) continue;

        std::string ext = entry.path().extension().string();
        for (char& c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        if (ext != ".dll") continue;

        ++totalDlls;
        const std::string fileName = entry.path().filename().string();

        UINT oldErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);
        ScopedModule module(LoadLibraryW(entry.path().wstring().c_str()));
        SetErrorMode(oldErrorMode);

        if (!module) {
            std::cerr << "[Skip] Файл '" << fileName
                << "' не является корректной DLL (код ошибки WinAPI: "
                << GetLastError() << ").\n";
            continue;
        }

        auto registerFunc = reinterpret_cast<RegisterPluginFunc>(
            GetProcAddress(module.get(), PLUGIN_ENTRY_SYMBOL)
            );

        if (!registerFunc) {
            std::cerr << "[Skip] Библиотека '" << fileName
                << "' не содержит точку входа '" << PLUGIN_ENTRY_SYMBOL << "'.\n";
            continue;
        }

        const PluginDescriptor* desc = nullptr;
        try {
            desc = registerFunc();
        }
        catch (...) {
            std::cerr << "[Skip] Исключение при инициализации плагина в '" << fileName << "'.\n";
            continue;
        }

        if (!desc || desc->abiVersion != PLUGIN_ABI_VERSION || !desc->functionName || !desc->execute) {
            std::cerr << "[Skip] Некорректная структура дескриптора или версия ABI в '" << fileName << "'.\n";
            continue;
        }

        std::string funcName(desc->functionName);
        if (!isValidIdentifier(funcName)) {
            std::cerr << "[Skip] Некорректное имя функции '" << funcName
                << "' в плагине '" << fileName << "'.\n";
            continue;
        }

        if (functions_.find(funcName) != functions_.end()) {
            std::cerr << "[Skip] Конфликт имён: функция '" << funcName
                << "' из '" << fileName << "' уже зарегистрирована.\n";
            continue;
        }

        PluginExecuteFunc rawExec = desc->execute;

        functions_[funcName] = [rawExec, funcName](double arg) -> double {
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
                throw std::domain_error("Ошибка в функции " + funcName + "(" + std::to_string(arg) + "): " + errMsg);
            }

            if (!std::isfinite(result)) {
                throw std::overflow_error("Функция " + funcName + " вернула бесконечность или NaN");
            }

            return result;
            };

        loadedModules_.push_back(std::move(module));
        ++loadedCount;

        std::cout << "[OK] Загружена функция '" << funcName
            << "' (источник: " << fileName << ")\n";
    }

    if (totalDlls == 0) {
        std::cout << "[Info] В папке '" << pluginDir.string()
            << "' нет ни одного .dll файла. Доступны базовые операции.\n";
    }
    else {
        std::cout << "[Info] Загружено плагинов: " << loadedCount << " из " << totalDlls << ".\n";
    }
}

bool PluginManager::hasFunction(const std::string& name) const {
    return functions_.find(name) != functions_.end();
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