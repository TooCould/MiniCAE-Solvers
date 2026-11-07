#pragma once

#include <vector>
#include <cstddef>

namespace minicae {
    class CoordinateMatrix {
    public:
        CoordinateMatrix() = default;
        CoordinateMatrix(std::size_t rows, std::size_t cols);

        std::size_t rows() const noexcept;
        std::size_t cols() const noexcept;
        std::size_t nnz() const noexcept;

        void insert(std::size_t row, std::size_t col, double value);

        const std::vector<std::size_t> &row_indices() const noexcept;
        const std::vector<std::size_t> &col_indices() const noexcept;
        const std::vector<double> &values() const noexcept;

        void canonicalize();
    private:
        std::size_t rows_ = 0;
        std::size_t cols_ = 0;
        std::vector<std::size_t> row_indices_;
        std::vector<std::size_t> col_indices_;
        std::vector<double> values_;
    };
} // namespace minicae