#include "linalg/coordinate_matrix.hpp"

#include <stdexcept>
#include <algorithm>  // std::sort
#include <numeric>    // std::iota

minicae::CoordinateMatrix::CoordinateMatrix(std::size_t rows, std::size_t cols)
    : rows_(rows), cols_(cols) {}

std::size_t minicae::CoordinateMatrix::rows() const noexcept { return rows_; }

std::size_t minicae::CoordinateMatrix::cols() const noexcept { return cols_; }

std::size_t minicae::CoordinateMatrix::nnz() const noexcept { return values_.size(); }

void minicae::CoordinateMatrix::insert(std::size_t row, std::size_t col, double value) {
    if (row >= rows_ || col >= cols_) {
        throw std::out_of_range("CoordinateMatrix::insert: row or column index out of range");
    }
    if (value == 0.0) {
        return;
    }
    row_indices_.push_back(row);
    col_indices_.push_back(col);
    values_.push_back(value);
}

const std::vector<std::size_t> &minicae::CoordinateMatrix::row_indices() const noexcept {
    return row_indices_;
}

const std::vector<std::size_t> &minicae::CoordinateMatrix::col_indices() const noexcept {
    return col_indices_;
}

const std::vector<double> &minicae::CoordinateMatrix::values() const noexcept { return values_; }

void minicae::CoordinateMatrix::canonicalize() {
    // TODO: Sort entries by (row, col).
    // TODO: Sum values with identical coordinates.
    // TODO: Remove zero entries and update all three arrays together.
    const std::size_t length = nnz();
    if (length != row_indices_.size() || length != col_indices_.size()) {
        throw std::logic_error("CoordinateMatrix::canonicalize: inconsistent sizes of row_indices, "
                               "col_indices, and values");
    }

    std::vector<std::size_t> order(length, 0);
    std::iota(order.begin(), order.end(), std::size_t{0});

    std::sort(order.begin(), order.end(), 
        [this](std::size_t a, std::size_t b) {
        if (row_indices_[a] != row_indices_[b]) {
            return row_indices_[a] < row_indices_[b];
        }
        if (col_indices_[a] != col_indices_[b]) {
            return col_indices_[a] < col_indices_[b];
        }
        return a < b;
        }
    );

    std::vector<std::size_t> sorted_rows;
    std::vector<std::size_t> sorted_cols;
    std::vector<double> sorted_values;
    sorted_rows.reserve(length);
    sorted_cols.reserve(length);
    sorted_values.reserve(length);

    for (std::size_t i = 0; i < length;) {
        std::size_t idx = order[i];
        const std::size_t row = row_indices_[idx];
        const std::size_t col = col_indices_[idx];
        double sum = values_[idx];
        std::size_t j = i;
        while (++j < length) {
            idx = order[j];
            if (row_indices_[idx] == row &&
                col_indices_[idx] == col) {
                sum += values_[idx];
            }
            else {
                break;
            }
        }
        if (sum) {
            sorted_rows.push_back(row);
            sorted_cols.push_back(col);
            sorted_values.push_back(sum);
        }
        i = j;
    }

    row_indices_.swap(sorted_rows);
    col_indices_.swap(sorted_cols);
    values_.swap(sorted_values);
}
