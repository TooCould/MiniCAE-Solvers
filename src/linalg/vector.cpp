#include "linalg/vector.hpp"
#include <cmath>
#include <stdexcept>

minicae::Vector::Vector(const std::size_t n, const double initial_value) : data_(n, initial_value) {}

minicae::Vector::Vector(std::initializer_list<double> values) : data_(values) {}

std::size_t minicae::Vector::size() const noexcept { return this->data_.size(); }

double &minicae::Vector::operator[](std::size_t i) {
    return this->data_.at(i);
}

const double &minicae::Vector::operator[](std::size_t i) const {
    return this->data_.at(i);
}

minicae::Vector minicae::operator+(const Vector &x, const Vector &y) {
    Vector result(x);
    axpy(1.0, y, result);
    return result;
}

minicae::Vector minicae::operator-(const Vector &x, const Vector &y) { 
    Vector result(x);
    axpy(-1.0, y, result);
    return result;
}

minicae::Vector minicae::operator*(double alpha, const Vector &x) { 
    Vector result(x);
    for (std::size_t i = 0; i < x.size(); ++i) {
        result[i] *= alpha;
    }
    return result;
}

minicae::Vector minicae::operator*(const Vector &x, double alpha) { 
    return alpha * x;
}

double minicae::dot(const Vector &x, const Vector &y) {
    if (x.size() != y.size()) {
        throw std::invalid_argument("Vectors must be of the same size for dot product.");
    }
    double result = 0.0;
    for (std::size_t i = 0; i < x.size(); ++i) {
        result += x[i] * y[i];
    }
    return result;
}

double minicae::norm2(const Vector &x) {
    double scale = 0.0;
    double ssq = 1.0;
    for (std::size_t i = 0; i < x.size(); ++i) {
        const double a = std::abs(x[i]);
        if (std::isnan(a)) {
            return a; // Return NaN if any element is NaN
        }
        if (std::isinf(a)) {
            scale = a;
            continue;
        }
        if (a == 0.0 || std::isinf(scale)) {
            continue;
        }
        if (a > scale) {
            const double ratio = scale / a;
            ssq = 1.0 + ssq * ratio * ratio;
            scale = a;
        } else {
            ssq += (a / scale) * (a / scale);
        }
    }
    return scale * std::sqrt(ssq);
}

minicae::Vector minicae::operator-(const Vector &x) { 
    Vector result(x);
    negate(result);
    return result;
}

void minicae::negate(Vector &x) { 
    for (std::size_t i = 0; i < x.size(); ++i) {
        x[i] = -x[i];
    }
}

void minicae::axpy(double alpha, const Vector &x, Vector &y) {
    if (x.size() != y.size()) {
        throw std::invalid_argument("Vectors must be of the same size for axpy operation.");
    }
    for (std::size_t i = 0; i < x.size(); ++i) {
        y[i] += alpha * x[i];
    }
}
