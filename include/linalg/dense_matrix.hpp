#pragma once

#include <cstddef> // std::size_t
#include <vector>  // std::vector
#include "vector.hpp"

namespace minicae {
    class DenseMatrix {
    public:
        DenseMatrix() = default;
        DenseMatrix(const std::size_t rows, const std::size_t cols, double initial_value = 0.0);

        std::size_t rows() const noexcept;
        std::size_t cols() const noexcept;

        double &operator()(const std::size_t row, const std::size_t col);
        const double &operator()(const std::size_t row, const std::size_t col) const;

        std::size_t checked_index(const std::size_t row, const std::size_t col) const;

    private:
        std::size_t rows_ = 0;
        std::size_t cols_ = 0;
        std::vector<double> data_;
    };

    DenseMatrix transpose(const DenseMatrix &A);
    Vector operator*(const DenseMatrix &A, const Vector &x);
    DenseMatrix operator*(const DenseMatrix &A, const DenseMatrix &B);

} // namespace minicae