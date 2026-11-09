#pragma once

#include <cstdint>
#include <cstddef>

#ifdef _WIN32
#define PLUGIN_API extern "C" __declspec(dllexport)
#define CALL_CONV __cdecl
#else
#define PLUGIN_API extern "C" __attribute__((visibility("default")))
#define CALL_CONV
#endif

constexpr std::uint32_t PLUGIN_ABI_VERSION = 1;

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

using RegisterPluginFunc = const PluginDescriptor* (CALL_CONV*)();

constexpr const char* PLUGIN_ENTRY_SYMBOL = "RegisterCalculatorPlugin";