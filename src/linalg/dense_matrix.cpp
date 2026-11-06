#include "linalg/dense_matrix.hpp"
#include <limits>
#include <stdexcept>

minicae::DenseMatrix::DenseMatrix(const std::size_t rows, const std::size_t cols, double initial_value)
    : rows_(rows), cols_(cols) {
    if (cols != 0 && rows > std::numeric_limits<std::size_t>::max() / cols) {
        throw std::length_error("Matrix dimensions overflow");
    }
    data_.resize(rows * cols, initial_value);
}

std::size_t minicae::DenseMatrix::rows() const noexcept { return rows_; }

std::size_t minicae::DenseMatrix::cols() const noexcept { return cols_; }

double &minicae::DenseMatrix::operator()(const std::size_t row, const std::size_t col) {
    return data_.at(checked_index(row, col));
}

const double &minicae::DenseMatrix::operator()(const std::size_t row, const std::size_t col) const {
    return data_.at(checked_index(row, col));
}

std::size_t minicae::DenseMatrix::checked_index(const std::size_t row, const std::size_t col) const {
    if (row >= rows_ || col >= cols_) {
        throw std::out_of_range("Index out of bounds");
    }
    return row * cols_ + col;
}

minicae::DenseMatrix minicae::transpose(const DenseMatrix &A) {
    DenseMatrix result(A.cols(), A.rows());
    for (std::size_t i = 0; i < A.rows(); ++i) {
        for (std::size_t j = 0; j < A.cols(); ++j) {
            result(j, i) = A(i, j);
        }
    }
    return result;
}

minicae::Vector minicae::operator*(const DenseMatrix &A, const Vector &x) {
    if (A.cols() != x.size()) {
        throw std::invalid_argument("Matrix columns must match vector size for multiplication.");
    }
    Vector result(A.rows(), 0.0);
    for (std::size_t i = 0; i < A.rows(); ++i) {
        for (std::size_t j = 0; j < A.cols(); ++j) {
            result[i] += A(i, j) * x[j];
        }
    }
    return result;
}

minicae::DenseMatrix minicae::operator*(const DenseMatrix &A, const DenseMatrix &B) {
    if (A.cols() != B.rows()) {
        throw std::invalid_argument(
            "Matrix A columns must match Matrix B rows for multiplication.");
    }
    DenseMatrix result(A.rows(), B.cols(), 0.0);
    for (std::size_t i = 0; i < A.rows(); ++i) {
        for (std::size_t j = 0; j < B.cols(); ++j) {
            for (std::size_t k = 0; k < A.cols(); ++k) {
                result(i, j) += A(i, k) * B(k, j);
            }
        }
    }
    return result;
}
