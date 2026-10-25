#include "plugin_manager.h"
#include "expression_parser.h"
#include <iostream>
#include <string>
#include <windows.h>


fs::path resolvePluginDirectory() {
    fs::path localPlugins = "./plugins";
    std::error_code ec;
    if (fs::exists(localPlugins, ec) && fs::is_directory(localPlugins, ec)) {
        return localPlugins;
    }

    wchar_t exePath[MAX_PATH] = { 0 };
    if (GetModuleFileNameW(nullptr, exePath, MAX_PATH) > 0) {
        fs::path exeDirPlugins = fs::path(exePath).parent_path() / "plugins";
        if (fs::exists(exeDirPlugins, ec) && fs::is_directory(exeDirPlugins, ec)) {
            return exeDirPlugins;
        }
    }
    return localPlugins;
}

int main() {
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);

    std::cout << "=== Консольный калькулятор с поддержкой динамических плагинов ===\n";

    PluginManager pluginManager;
    pluginManager.loadFromDirectory(resolvePluginDirectory());

    auto funcs = pluginManager.getRegisteredFunctionNames();
    if (!funcs.empty()) {
        std::cout << "Доступные функции из плагинов: ";
        for (std::size_t i = 0; i < funcs.size(); ++i) {
            std::cout << funcs[i] << (i + 1 < funcs.size() ? ", " : "\n");
        }
    }

    std::cout << "Команды: 'help' — справка, 'exit' — выход.\n\n";

    ExpressionEvaluator evaluator(pluginManager);
    std::string line;

    while (true) {
        std::cout << "Ввод > ";
        if (!std::getline(std::cin, line)) {
            break;
        }

        if (line.find_first_not_of(" \t\r\n") == std::string::npos) {
            continue;
        }

        if (line == "exit" || line == "quit") {
            break;
        }

        if (line == "help") {
            std::cout << "Поддерживаемые операции: +, -, *, /, ^, ()\n"
                << "Примеры:\n"
                << "  16 + 4 * (3 - 1)\n"
                << "  2^4 + sin(90)\n"
                << "  ln(exp(1))\n";
            continue;
        }

        try {
            double answer = evaluator.evaluate(line);
            std::cout << "Вывод: " << answer << "\n\n";
        }
        catch (const std::exception& ex) {
            std::cout << "[Ошибка] " << ex.what() << "\n\n";
        }
    }

    return 0;
}