#include "plugin_helpers.h"

namespace {
    double computeLn(double x) {
        if (x <= 0.0) {
            throw std::domain_error("Натуральный логарифм ln(x) определен только при x > 0");
        }
        return std::log(x);
    }
}

DEFINE_CALC_PLUGIN("ln", computeLn)
