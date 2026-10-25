#pragma once

#include "plugin_api.h"
#include <windows.h>
#include <filesystem>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include <cctype>

namespace fs = std::filesystem;

// RAII-обёртка над HMODULE: гарантирует вызов FreeLibrary при выходе из области видимости
// или при возникновении любого исключения.
class ScopedModule {
private:
    HMODULE handle_ = nullptr;

public:
    ScopedModule() = default;
    explicit ScopedModule(HMODULE h) noexcept : handle_(h) {}

    ~ScopedModule() noexcept {
        reset();
    }

    // Запрещаем копирование (уникальное владение ресурсом ОС)
    ScopedModule(const ScopedModule&) = delete;
    ScopedModule& operator=(const ScopedModule&) = delete;

    // Разрешаем перемещение
    ScopedModule(ScopedModule&& other) noexcept : handle_(other.handle_) {
        other.handle_ = nullptr;
    }

    ScopedModule& operator=(ScopedModule&& other) noexcept {
        if (this != &other) {
            reset();
            handle_ = other.handle_;
            other.handle_ = nullptr;
        }
        return *this;
    }

    void reset(HMODULE newHandle = nullptr) noexcept {
        if (handle_) {
            FreeLibrary(handle_);
        }
        handle_ = newHandle;
    }

    HMODULE get() const noexcept { return handle_; }
    explicit operator bool() const noexcept { return handle_ != nullptr; }
};

class PluginManager {
public:
    using UnaryMathFunc = std::function<double(double)>;

private:
    // Важно: сначала объявляем контейнер дескрипторов DLL, а затем словарь функций.
    // По правилам C++ поля класса разрушаются в обратном порядке:
    // сначала очистятся std::function, и только потом выгрузятся DLL через ~ScopedModule().
    std::vector<ScopedModule> loadedModules_;
    std::map<std::string, UnaryMathFunc> functions_;

    static bool isValidIdentifier(const std::string& name) {
        if (name.empty()) return false;
        unsigned char first = static_cast<unsigned char>(name[0]);
        if (!std::isalpha(first) && first != '_') return false;

        for (unsigned char ch : name) {
            if (!std::isalnum(ch) && ch != '_') return false;
        }
        return true;
    }

public:
    PluginManager() = default;
    ~PluginManager() = default;

    PluginManager(const PluginManager&) = delete;
    PluginManager& operator=(const PluginManager&) = delete;

    // Загрузка всех плагинов из папки ./plugins
    void loadFromDirectory(const fs::path& pluginDir = "./plugins") {
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

            // Сравниваем расширение без учёта регистра (.dll / .DLL)
            std::string ext = entry.path().extension().string();
            for (char& c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            if (ext != ".dll") continue;

            ++totalDlls;
            const std::string fileName = entry.path().filename().string();

            // Подавляем системные диалоговые окна Windows при загрузке битых DLL
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
                continue; // ScopedModule автоматически вызовет FreeLibrary
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
                    << "' из '" << fileName << "' уже зарегистрирована другим плагином.\n";
                continue;
            }

            PluginExecuteFunc rawExec = desc->execute;

            // Регистрируем безопасную обёртку
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

            // Перемещаем владение DLL в вектор загруженных модулей
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

    bool hasFunction(const std::string& name) const {
        return functions_.find(name) != functions_.end();
    }

    double callFunction(const std::string& name, double arg) const {
        auto it = functions_.find(name);
        if (it == functions_.end()) {
            throw std::invalid_argument("Неизвестная функция: '" + name + "'");
        }
        return it->second(arg);
    }

    std::vector<std::string> getRegisteredFunctionNames() const {
        std::vector<std::string> names;
        names.reserve(functions_.size());
        for (const auto& [name, _] : functions_) {
            names.push_back(name);
        }
        return names;
    }
};
