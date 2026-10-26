#pragma once

#include "plugin_manager.h"
#include <cstddef>
#include <string>

class ExpressionEvaluator {
private:
    const PluginManager& pluginManager_;
    std::string input_;
    std::size_t pos_ = 0;

    void skipWhitespace();
    bool match(char expected);
    double parseExpression();
    double parseTerm();
    double parseUnary();
    double parsePower();
    double parsePrimary();
    std::string parseIdentifier();
    double parseNumber();
    static std::string normalizeInput(const std::string& raw);

public:
    explicit ExpressionEvaluator(const PluginManager& pm);
    double evaluate(const std::string& rawExpression);
};