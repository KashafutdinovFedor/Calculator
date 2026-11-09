#include "plugin_helpers.h"

namespace {
    double computeExp(double x) {
        double res = std::exp(x);
        if (!std::isfinite(res)) {
            throw std::overflow_error("Переполнение при вычислении exp(x)");
        }
        return res;
    }
}

DEFINE_CALC_PLUGIN("exp", computeExp)
