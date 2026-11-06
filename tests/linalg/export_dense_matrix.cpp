#include "linalg/dense_matrix.hpp"

#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <locale>
#include <stdexcept>
#include <string>

namespace {
minicae::DenseMatrix read_matrix(std::size_t rows, std::size_t cols)
{
    if (rows > 1000000 || cols > 1000000 || (cols != 0 && rows > 1000000 / cols)) {
        throw std::runtime_error("Validation matrix exceeds element limit");
    }
    minicae::DenseMatrix result(rows, cols);
    for (std::size_t i = 0; i < rows; ++i) {
        for (std::size_t j = 0; j < cols; ++j) {
            if (!(std::cin >> result(i, j)) || !std::isfinite(result(i, j))) {
                throw std::runtime_error("Expected finite matrix elements");
            }
        }
    }
    return result;
}

void export_matmul()
{
    std::size_t rows = 0, inner = 0, cols = 0;
    if (!(std::cin >> rows >> inner >> cols) || rows > 1000000 || cols > 1000000 ||
        (cols != 0 && rows > 1000000 / cols)) {
        throw std::runtime_error("Expected bounded dimensions m, k, n");
    }
    const auto a = read_matrix(rows, inner);
    const auto b = read_matrix(inner, cols);
    const auto product = a * b;
    std::cout << "operation,index,value\n";
    std::cout << "rows,0," << a.rows() << "\ncols,0," << a.cols() << '\n';
    std::cout << "b_rows,0," << b.rows() << "\nb_cols,0," << b.cols() << '\n';
    std::cout << "product_rows,0," << product.rows() << '\n';
    std::cout << "product_cols,0," << product.cols() << '\n';
    std::size_t index = 0;
    for (std::size_t i = 0; i < product.rows(); ++i) {
        for (std::size_t j = 0; j < product.cols(); ++j) {
            std::cout << "matmul," << index++ << ',' << product(i, j) << '\n';
        }
    }
}
}

int main(int argc, char* argv[])
{
    try {
        std::cin.imbue(std::locale::classic());
        std::cout.imbue(std::locale::classic());
        std::cout << std::setprecision(std::numeric_limits<double>::max_digits10);
        if (argc == 2 && std::string(argv[1]) == "--matmul") {
            export_matmul();
            return 0;
        }
        if (argc != 1) {
            throw std::runtime_error("Usage: export_dense_matrix [--matmul]");
        }
        std::size_t rows = 0, cols = 0;
        if (!(std::cin >> rows >> cols) || rows > 1000000 || cols > 1000000 ||
            (cols != 0 && rows > 1000000 / cols)) {
            throw std::runtime_error("Expected nonnegative dimensions, at most 1000000 elements");
        }
        minicae::DenseMatrix matrix(rows, cols);
        for (std::size_t i = 0; i < rows; ++i) {
            for (std::size_t j = 0; j < cols; ++j) {
                if (!(std::cin >> matrix(i, j)) || !std::isfinite(matrix(i, j))) {
                    throw std::runtime_error("Expected finite matrix elements in row-major order");
                }
            }
        }
        minicae::Vector x(cols);
        for (std::size_t j = 0; j < cols; ++j) {
            if (!(std::cin >> x[j]) || !std::isfinite(x[j])) {
                throw std::runtime_error("Expected finite vector elements");
            }
        }
        const auto transposed = minicae::transpose(matrix);
        const auto product = matrix * x;
        std::cout << std::setprecision(std::numeric_limits<double>::max_digits10);
        std::cout << "operation,index,value\n";
        std::cout << "rows,0," << rows << "\ncols,0," << cols << '\n';
        std::cout << "transpose_rows,0," << transposed.rows() << '\n';
        std::cout << "transpose_cols,0," << transposed.cols() << '\n';
        std::cout << "product_size,0," << product.size() << '\n';
        std::size_t index = 0;
        for (std::size_t i = 0; i < transposed.rows(); ++i) {
            for (std::size_t j = 0; j < transposed.cols(); ++j) {
                std::cout << "transpose," << index++ << ',' << transposed(i, j) << '\n';
            }
        }
        for (std::size_t i = 0; i < product.size(); ++i) {
            std::cout << "matvec," << i << ',' << product[i] << '\n';
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
