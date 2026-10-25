#pragma once

#include <cstdint>
#include <cstddef>

#ifdef _WIN32
#define PLUGIN_API extern "C" __declspec(dllexport)
#define CALL_CONV __cdecl
#else
#define PLUGIN_API extern "C"
#define CALL_CONV
#endif

// Версия ABI интерфейса для проверки совместимости DLL
constexpr std::uint32_t PLUGIN_ABI_VERSION = 1;

// Указатель на вычислительную функцию внутри плагина:
// x - входной аргумент
// outResult - указатель для записи результата
// errorBuf - буфер для записи текста исключения/ошибки
// errorBufSize - размер буфера
// Возвращает 0 при успехе, иначе код ошибки
using PluginExecuteFunc = int (CALL_CONV*)(
    double x,
    double* outResult,
    char* errorBuf,
    std::size_t errorBufSize
    );

struct PluginDescriptor {
    std::uint32_t abiVersion;
    const char* functionName;      
    PluginExecuteFunc execute;     
};

// Тип экспортируемой функции получения метаданных плагина
using RegisterPluginFunc = const PluginDescriptor* (CALL_CONV*)();

// Имя точки входа в DLL
constexpr const char* PLUGIN_ENTRY_SYMBOL = "RegisterCalculatorPlugin";