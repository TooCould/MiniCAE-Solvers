#include "linalg/vector.hpp"

#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <locale>
#include <stdexcept>

namespace {
void write_vector(const char* operation, const minicae::Vector& values)
{
    for (std::size_t i = 0; i < values.size(); ++i) {
        std::cout << operation << ',' << i << ',' << values[i] << '\n';
    }
}
}

int main()
{
    try {
        std::cin.imbue(std::locale::classic());
        std::cout.imbue(std::locale::classic());
        std::size_t size = 0;
        double alpha = 0.0;
        if (!(std::cin >> size >> alpha) || size > 1000000 || !std::isfinite(alpha)) {
            throw std::runtime_error("Expected size (0..1000000) and finite alpha");
        }
        minicae::Vector x(size), y(size);
        for (std::size_t i = 0; i < size; ++i) {
            if (!(std::cin >> x[i]) || !std::isfinite(x[i])) {
                throw std::runtime_error("Expected finite x elements");
            }
        }
        for (std::size_t i = 0; i < size; ++i) {
            if (!(std::cin >> y[i]) || !std::isfinite(y[i])) {
                throw std::runtime_error("Expected finite y elements");
            }
        }
        std::cout << std::setprecision(std::numeric_limits<double>::max_digits10);
        std::cout << "operation,index,value\nsize,0," << size << '\n';
        write_vector("add", x + y);
        write_vector("subtract", x - y);
        write_vector("scale_left", alpha * x);
        write_vector("scale_right", x * alpha);
        write_vector("negate", -x);
        minicae::Vector result(y);
        minicae::axpy(alpha, x, result);
        write_vector("axpy", result);
        std::cout << "dot,0," << minicae::dot(x, y) << '\n';
        std::cout << "norm2,0," << minicae::norm2(x) << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
