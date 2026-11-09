#pragma once

#include "plugin_api.h"
#include <cmath>
#include <cstring>
#include <exception>
#include <stdexcept>
#include <string>

namespace plugin_helpers {

    using ComputeFunc = double (*)(double);

    inline const double kPi = std::acos(-1.0);

    inline void requireFinite(double x, const char* funcName) {
        if (!std::isfinite(x)) {
            throw std::domain_error(std::string("Аргумент ") + funcName + " не является конечным числом");
        }
    }

    inline void copyError(char* errorBuf, std::size_t errorBufSize, const char* message) {
        if (!errorBuf || errorBufSize == 0) return;
        std::size_t len = std::strlen(message);
        if (len >= errorBufSize) len = errorBufSize - 1;
        std::memcpy(errorBuf, message, len);
        errorBuf[len] = '\0';
    }

    template <ComputeFunc Compute>
    int CALL_CONV safeExecute(double x, double* outResult, char* errorBuf, std::size_t errorBufSize) {
        if (!outResult) return -1;
        try {
            *outResult = Compute(x);
            return 0;
        }
        catch (const std::exception& e) {
            copyError(errorBuf, errorBufSize, e.what());
        }
        catch (...) {
            copyError(errorBuf, errorBufSize, "неизвестная ошибка в плагине");
        }
        return 1;
    }

} 
#define DEFINE_CALC_PLUGIN(NAME, COMPUTE)                                              \
    namespace {                                                                        \
        const PluginDescriptor kDescriptor = {                                         \
            PLUGIN_ABI_VERSION,                                                        \
            NAME,                                                                      \
            &plugin_helpers::safeExecute<COMPUTE>                                      \
        };                                                                             \
    }                                                                                  \
    PLUGIN_API const PluginDescriptor* CALL_CONV RegisterCalculatorPlugin() {          \
        return &kDescriptor;                                                           \
    }
