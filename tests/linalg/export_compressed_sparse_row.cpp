#include "linalg/compressed_sparse_row.hpp"
#include "linalg/coordinate_matrix.hpp"

#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <locale>
#include <stdexcept>

int main() {
    try {
        std::cin.imbue(std::locale::classic());
        std::cout.imbue(std::locale::classic());
        std::size_t rows=0, cols=0, count=0;
        double atol=0;
        if (!(std::cin >> rows >> cols >> count >> atol) ||
            rows>1000000 || cols>1000000 || count>1000000 ||
            !std::isfinite(atol) || atol<0) {
            throw std::runtime_error("Expected bounded shape/count and finite nonnegative atol");
        }
        minicae::CoordinateMatrix coo(rows,cols);
        for (std::size_t i=0; i<count; ++i) {
            std::size_t row=0, col=0;
            double value=0;
            if (!(std::cin >> row >> col >> value) || !std::isfinite(value))
                throw std::runtime_error("Expected finite COO entry");
            coo.insert(row,col,value);
        }
        minicae::Vector x(cols);
        for (std::size_t i=0; i<cols; ++i) {
            if (!(std::cin >> x[i]) || !std::isfinite(x[i]))
                throw std::runtime_error("Expected finite vector element");
        }
        std::cin >> std::ws;
        if (!std::cin.eof()) throw std::runtime_error("Unexpected trailing input");
        const minicae::CompressedSparseRow a(coo);
        a.validate();
        const auto y=a*x;
        const auto d=minicae::diagonal(a);
        std::cout << std::setprecision(std::numeric_limits<double>::max_digits10);
        std::cout << "type,index,value\n";
        std::cout << "rows,0," << a.rows() << "\ncols,0," << a.cols()
                  << "\nnnz,0," << a.nnz() << '\n';
        for (std::size_t i=0; i<a.row_ptr().size(); ++i)
            std::cout << "row_ptr," << i << ',' << a.row_ptr()[i] << '\n';
        for (std::size_t i=0; i<a.nnz(); ++i) {
            std::cout << "col_indices," << i << ',' << a.col_indices()[i] << '\n';
            std::cout << "values," << i << ',' << a.values()[i] << '\n';
        }
        for (std::size_t i=0; i<y.size(); ++i)
            std::cout << "matvec," << i << ',' << y[i] << '\n';
        for (std::size_t i=0; i<d.size(); ++i)
            std::cout << "diagonal," << i << ',' << d[i] << '\n';
        std::cout << "norm_inf,0," << minicae::norm_inf(a) << '\n';
        std::cout << "symmetric,0," << minicae::is_symmetric(a,atol) << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
