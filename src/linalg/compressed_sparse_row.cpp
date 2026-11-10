#include "linalg/compressed_sparse_row.hpp"
#include "linalg/coordinate_matrix.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace minicae {
    CompressedSparseRow::CompressedSparseRow(std::size_t rows, std::size_t cols)
        : rows_{rows}, cols_{cols} {
        if (rows == std::numeric_limits<std::size_t>::max()) {
            throw std::length_error("CompressedSparseRow: row pointer size overflow");
        }
        row_ptr_.assign(rows + 1, 0);
    }

    CompressedSparseRow::CompressedSparseRow(const CoordinateMatrix &matrix)
        : CompressedSparseRow(matrix.rows(), matrix.cols()) {
        CoordinateMatrix canonical_coo(matrix);
        canonical_coo.canonicalize();
        col_indices_ = canonical_coo.col_indices();
        values_ = canonical_coo.values();
        for (std::size_t i : canonical_coo.row_indices()) {
            ++row_ptr_[i + 1];
        }
        for (std::size_t i = 0; i < rows_; ++i) {
            row_ptr_[i + 1] += row_ptr_[i];
        }
    }

    CompressedSparseRow::CompressedSparseRow(std::size_t rows, std::size_t cols,
                                             const std::vector<std::size_t> &row_ptr,
                                             const std::vector<std::size_t> &col_indices,
                                             const std::vector<double> &values)
        : rows_{rows}, cols_{cols}, row_ptr_{row_ptr}, col_indices_{col_indices}, values_{values} {
        validate();
    }

    std::size_t CompressedSparseRow::rows() const noexcept { return rows_; }
    std::size_t CompressedSparseRow::cols() const noexcept { return cols_; }
    std::size_t CompressedSparseRow::nnz() const noexcept { return values_.size(); }

    const std::vector<std::size_t> &CompressedSparseRow::row_ptr() const noexcept {
        return row_ptr_;
    }
    const std::vector<std::size_t> &CompressedSparseRow::col_indices() const noexcept {
        return col_indices_;
    }
    const std::vector<double> &CompressedSparseRow::values() const noexcept { return values_; }

    void CompressedSparseRow::validate() const {
        if (rows_ == std::numeric_limits<std::size_t>::max()) {
            throw std::invalid_argument("CompressedSparseRow: row pointer size overflow");
        }
        const std::size_t num = nnz();

        // 1. 先检查长度，再访问首尾指针。
        if (row_ptr_.size() != rows_ + 1 || col_indices_.size() != num) {
            throw std::invalid_argument("CompressedSparseRow: inconsistent array lengths");
        }
        if (row_ptr_.front() != 0 || row_ptr_.back() != num) {
            throw std::invalid_argument("CompressedSparseRow: invalid first or last row pointer");
        }

        // 2. 全部指针单调且首尾合法，才能保证各行范围都在 [0, nnz] 内。
        for (std::size_t i = 0; i < rows_; ++i) {
            if (row_ptr_[i] > row_ptr_[i + 1]) {
                throw std::invalid_argument(
                    "CompressedSparseRow: row pointers must be nondecreasing");
            }
        }

        // 3. 检查每个条目；空行自然跳过，单条目行也会被检查。
        for (std::size_t i = 0; i < rows_; ++i) {
            const std::size_t begin = row_ptr_[i];
            const std::size_t end = row_ptr_[i + 1];
            for (std::size_t j = begin; j < end; ++j) {
                if (col_indices_[j] >= cols_) {
                    throw std::invalid_argument("CompressedSparseRow: column index out of range");
                }
                if (values_[j] == 0.0) {
                    throw std::invalid_argument("CompressedSparseRow: explicit zero value");
                }
                if (j > begin && col_indices_[j - 1] >= col_indices_[j]) {
                    throw std::invalid_argument(
                        "CompressedSparseRow: columns must be strictly increasing within each row");
                }
            }
        }
    }

    Vector operator*(const CompressedSparseRow &A, const Vector &x) {
        if (A.cols() != x.size()) {
            throw std::invalid_argument("CompressedSparseRow: matrix/vector dimensions mismatch");
        }
        Vector result(A.rows());
        for (std::size_t i = 0; i < A.rows(); ++i) {
            for (std::size_t j = A.row_ptr()[i]; j < A.row_ptr()[i + 1]; ++j) {
                result[i] += A.values()[j] * x[A.col_indices()[j]];
            }
        }
        return result;
    }

    Vector diagonal(const CompressedSparseRow &A) {
        Vector result(std::min(A.rows(), A.cols()));
        for (std::size_t i = 0; i < result.size(); ++i) {
            for (std::size_t j = A.row_ptr()[i]; j < A.row_ptr()[i + 1]; ++j) {
                if (A.col_indices()[j] == i) {
                    result[i] = A.values()[j];
                    break;
                }
            }
        }
        return result;
    }

    double norm_inf(const CompressedSparseRow &A) {
        if (A.rows() == 0 || A.cols() == 0) {
            return 0.0;
        }
        double result = 0.0;
        for (std::size_t i = 0; i < A.rows(); ++i) {
            double sum = 0.0;
            for (std::size_t j = A.row_ptr()[i]; j < A.row_ptr()[i + 1]; ++j) {
                sum += std::abs(A.values()[j]);
            }
            if (sum > result) {
                result = sum;
            }
        }
        return result;
    }

    bool is_symmetric(const CompressedSparseRow &A, double atol) {
        // atol 必须有限且非负，否则抛出 std::invalid_argument。
        if (!std::isfinite(atol) || atol < 0) {
            throw std::invalid_argument("is_symmetric: atol must be finite and nonnegative");
        }
        // 非方阵返回 false，0×0 矩阵返回 true；
        if (A.rows() != A.cols()) {
            return false;
        }
        // 逐项检查 |A(i,j) - A(j,i)| <= atol，缺失条目视为 0。
        for (std::size_t i = 0; i < A.rows(); ++i) {
            for (std::size_t j = A.row_ptr()[i]; j < A.row_ptr()[i + 1]; ++j) {
                // 矩阵包含 NaN 或无穷值时返回 false
                const double value = A.values()[j];
                if (!std::isfinite(value)) {
                    return false;
                }
                const std::size_t col = A.col_indices()[j];
                bool found = false;
                for (std::size_t k = A.row_ptr()[col]; k < A.row_ptr()[col + 1]; ++k) {
                    if (!std::isfinite(A.values()[k])) {
                        return false;
                    }
                    if (A.col_indices()[k] == i) {
                        if (std::abs(value - A.values()[k]) > atol) {
                            return false;
                        }
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    if (std::abs(value) > atol) {
                        return false;
                    }
                }
            }
        }
        return true;
    }

} // namespace minicae
