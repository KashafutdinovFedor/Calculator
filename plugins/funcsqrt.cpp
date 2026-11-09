#include "plugin_helpers.h"

namespace {
    double computeSqrt(double x) {
        if (x < 0.0) {
            throw std::domain_error("Квадратный корень sqrt(x) не определен для отрицательных чисел (x < 0)");
        }
        return std::sqrt(x);
    }
}

DEFINE_CALC_PLUGIN("sqrt", computeSqrt)
