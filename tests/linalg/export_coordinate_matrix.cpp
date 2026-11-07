#include "linalg/coordinate_matrix.hpp"

#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <locale>
#include <stdexcept>

int main()
{
    try {
        std::cin.imbue(std::locale::classic());
        std::cout.imbue(std::locale::classic());
        std::size_t rows = 0, cols = 0, count = 0;
        if (!(std::cin >> rows >> cols >> count) || count > 1000000) {
            throw std::runtime_error("Expected rows, cols and entry count (at most 1000000)");
        }
        minicae::CoordinateMatrix matrix(rows, cols);
        for (std::size_t i = 0; i < count; ++i) {
            std::size_t row = 0, col = 0;
            double value = 0.0;
            if (!(std::cin >> row >> col >> value) || !std::isfinite(value)) {
                throw std::runtime_error("Expected row, col and finite value for each entry");
            }
            matrix.insert(row, col, value);
        }
        matrix.canonicalize();
        std::cout << std::setprecision(std::numeric_limits<double>::max_digits10);
        std::cout << "type,row,col,value\n";
        std::cout << "shape," << matrix.rows() << ',' << matrix.cols() << ',' << matrix.nnz() << '\n';
        for (std::size_t i = 0; i < matrix.nnz(); ++i) {
            std::cout << "entry," << matrix.row_indices()[i] << ','
                      << matrix.col_indices()[i] << ',' << matrix.values()[i] << '\n';
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
