#include "expression_parser.h"
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <stdexcept>

ExpressionEvaluator::ExpressionEvaluator(const PluginManager& pm)
    : pluginManager_(pm) {}

void ExpressionEvaluator::skipWhitespace() {
    while (pos_ < input_.size() && std::isspace(static_cast<unsigned char>(input_[pos_]))) {
        ++pos_;
    }
}

bool ExpressionEvaluator::match(char expected) {
    skipWhitespace();
    if (pos_ < input_.size() && input_[pos_] == expected) {
        ++pos_;
        return true;
    }
    return false;
}

double ExpressionEvaluator::parseExpression() {
    double left = parseTerm();
    while (true) {
        if (match('+')) {
            left += parseTerm();
        }
        else if (match('-')) {
            left -= parseTerm();
        }
        else {
            break;
        }
    }
    return left;
}

double ExpressionEvaluator::parseTerm() {
    double left = parseUnary();
    while (true) {
        if (match('*')) {
            left *= parseUnary();
        }
        else if (match('/')) {
            double divisor = parseUnary();
            if (divisor == 0.0) {
                throw std::domain_error("Деление на ноль");
            }
            left /= divisor;
        }
        else {
            break;
        }
    }
    return left;
}

double ExpressionEvaluator::parseUnary() {
    if (match('+')) {
        return parseUnary();
    }
    if (match('-')) {
        return -parseUnary();
    }
    return parsePower();
}

double ExpressionEvaluator::parsePower() {
    double base = parsePrimary();
    if (match('^')) {
        double exponent = parseUnary();
        double result = std::pow(base, exponent);
        if (!std::isfinite(result)) {
            throw std::domain_error("Ошибка возведения в степень (комплексный результат или переполнение)");
        }
        return result;
    }
    return base;
}

double ExpressionEvaluator::parsePrimary() {
    skipWhitespace();

    if (match('(')) {
        if (match(')')) {
            throw std::invalid_argument("Пустые скобки '()' недопустимы");
        }
        double val = parseExpression();
        if (!match(')')) {
            throw std::invalid_argument("Пропущена закрывающая скобка ')'");
        }
        return val;
    }

    if (pos_ < input_.size()) {
        unsigned char ch = static_cast<unsigned char>(input_[pos_]);
        if (std::isalpha(ch) || ch == '_') {
            std::string funcName = parseIdentifier();
            if (!match('(')) {
                throw std::invalid_argument("Ожидалась открывающая скобка '(' после имени функции '" + funcName + "'");
            }
            if (match(')')) {
                throw std::invalid_argument("Функция '" + funcName + "' требует ровно один аргумент");
            }
            double arg = parseExpression();
            if (!match(')')) {
                throw std::invalid_argument("Пропущена закрывающая скобка ')' при вызове функции '" + funcName + "'");
            }
            return pluginManager_.callFunction(funcName, arg);
        }
    }

    return parseNumber();
}

std::string ExpressionEvaluator::parseIdentifier() {
    skipWhitespace();
    std::size_t start = pos_;
    while (pos_ < input_.size()) {
        unsigned char ch = static_cast<unsigned char>(input_[pos_]);
        if (!std::isalnum(ch) && ch != '_') break;
        ++pos_;
    }
    return input_.substr(start, pos_ - start);
}

double ExpressionEvaluator::parseNumber() {
    skipWhitespace();
    if (pos_ >= input_.size()) {
        throw std::invalid_argument("Неожиданный конец выражения (ожидалось число или подвыражение)");
    }

    const char* startPtr = input_.c_str() + pos_;
    char* endPtr = nullptr;
    double value = std::strtod(startPtr, &endPtr);

    if (startPtr == endPtr) {
        throw std::invalid_argument(
            std::string("Неожиданный символ '") + input_[pos_] + "' на позиции " + std::to_string(pos_ + 1)
        );
    }

    pos_ += static_cast<std::size_t>(endPtr - startPtr);
    if (!std::isfinite(value)) {
        throw std::overflow_error("Числовое значение выходит за пределы допустимого диапазона double");
    }
    return value;
}

std::string ExpressionEvaluator::normalizeInput(const std::string& raw) {
    std::string out;
    out.reserve(raw.size());
    for (std::size_t i = 0; i < raw.size(); ++i) {
        if (i + 2 < raw.size() &&
            static_cast<unsigned char>(raw[i]) == 0xE2 &&
            static_cast<unsigned char>(raw[i + 1]) == 0x80 &&
            (static_cast<unsigned char>(raw[i + 2]) == 0x93 || static_cast<unsigned char>(raw[i + 2]) == 0x94)) {
            out.push_back('-');
            i += 2;
        }
        else if (static_cast<unsigned char>(raw[i]) == 0x96 || static_cast<unsigned char>(raw[i]) == 0x97) {
            out.push_back('-');
        }
        else {
            out.push_back(raw[i]);
        }
    }
    return out;
}

double ExpressionEvaluator::evaluate(const std::string& rawExpression) {
    input_ = normalizeInput(rawExpression);
    pos_ = 0;

    double result = parseExpression();
    skipWhitespace();

    if (pos_ < input_.size()) {
        throw std::invalid_argument(
            std::string("Лишний или некорректный символ '") + input_[pos_] +
            "' на позиции " + std::to_string(pos_ + 1)
        );
    }

    if (!std::isfinite(result)) {
        throw std::overflow_error("Результат вычисления выходит за допустимые пределы");
    }

    if (std::abs(result) < 1e-15) {
        result = 0.0;
    }
    return result;
}