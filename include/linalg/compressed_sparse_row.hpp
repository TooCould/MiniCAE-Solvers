#pragma once

#include <cstddef>
#include <vector>
#include "vector.hpp"

namespace minicae {
    class CoordinateMatrix;

    class CompressedSparseRow {
    public:
        // 空的 0 × 0 矩阵，row_ptr 为 {0}。
        CompressedSparseRow() = default;
        CompressedSparseRow(std::size_t rows, std::size_t cols);

        // 从 COO 转换：排序、合并重复项并清理零值，不修改输入。
        explicit CompressedSparseRow(const CoordinateMatrix &matrix);

        // 从已有 CSR 数组构造；检查结构，不自动整理输入。
        // 每行列索引必须严格递增，values 中不存储显式零值。
        CompressedSparseRow(std::size_t rows, std::size_t cols,
                            const std::vector<std::size_t> &row_ptr,
                            const std::vector<std::size_t> &col_indices,
                            const std::vector<double> &values);

        std::size_t rows() const noexcept;
        std::size_t cols() const noexcept;
        std::size_t nnz() const noexcept;

        // 第 i 行的条目位于 [row_ptr[i], row_ptr[i + 1])。
        const std::vector<std::size_t> &row_ptr() const noexcept;
        const std::vector<std::size_t> &col_indices() const noexcept;
        const std::vector<double> &values() const noexcept;

        // 检查数组长度、首尾指针、指针单调性、列边界及行内顺序。
        // 结构非法或包含显式零值时抛出 std::invalid_argument。
        void validate() const;

    private:
        std::size_t rows_ = 0;
        std::size_t cols_ = 0;
        std::vector<std::size_t> row_ptr_{0};
        std::vector<std::size_t> col_indices_;
        std::vector<double> values_;
    };

    // 稀疏矩阵向量乘法；尺寸不匹配时抛出 std::invalid_argument。
    Vector operator*(const CompressedSparseRow &A, const Vector &x);

    // 提取主对角线，长度为 min(rows, cols)，未存储的对角元素返回 0。
    Vector diagonal(const CompressedSparseRow &A);

    // 矩阵无穷范数：各行元素绝对值之和的最大值；空矩阵返回 0。
    double norm_inf(const CompressedSparseRow &A);

    // 逐项检查 |A(i,j) - A(j,i)| <= atol，缺失条目视为 0。
    // 非方阵返回 false，0×0 矩阵返回 true；atol 默认 0，表示精确比较。
    // atol 必须有限且非负，否则抛出 std::invalid_argument。
    // 矩阵包含 NaN 或无穷值时返回 false。
    bool is_symmetric(const CompressedSparseRow &A, double atol = 0.0);

} // namespace minicae

