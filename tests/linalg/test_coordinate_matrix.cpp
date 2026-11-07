#include "linalg/coordinate_matrix.hpp"

#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

using minicae::CoordinateMatrix;

static_assert(std::is_same_v<decltype(std::declval<const CoordinateMatrix&>().row_indices()),
                             const std::vector<std::size_t>&>);
static_assert(std::is_same_v<decltype(std::declval<const CoordinateMatrix&>().col_indices()),
                             const std::vector<std::size_t>&>);
static_assert(std::is_same_v<decltype(std::declval<const CoordinateMatrix&>().values()),
                             const std::vector<double>&>);
static_assert(noexcept(std::declval<const CoordinateMatrix&>().rows()));
static_assert(noexcept(std::declval<const CoordinateMatrix&>().cols()));
static_assert(noexcept(std::declval<const CoordinateMatrix&>().nnz()));

namespace {

void require(bool condition, const char* message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void consistent(const CoordinateMatrix& matrix)
{
    require(matrix.row_indices().size() == matrix.nnz() &&
            matrix.col_indices().size() == matrix.nnz() && matrix.values().size() == matrix.nnz(),
            "Coordinate arrays must have identical lengths");
}

void same(const CoordinateMatrix& actual, const CoordinateMatrix& expected)
{
    consistent(actual);
    consistent(expected);
    require(actual.rows() == expected.rows() && actual.cols() == expected.cols(), "Shape mismatch");
    require(actual.row_indices() == expected.row_indices(), "Row indices changed");
    require(actual.col_indices() == expected.col_indices(), "Column indices changed");
    require(actual.values() == expected.values(), "Values changed");
}

template<class Function>
void out_of_range(Function function)
{
    try {
        function();
    } catch (const std::out_of_range&) {
        return;
    }
    throw std::runtime_error("Expected std::out_of_range was not thrown");
}

void construction()
{
    const CoordinateMatrix empty;
    require(empty.rows() == 0 && empty.cols() == 0 && empty.nnz() == 0, "Invalid default matrix");
    consistent(empty);
    const CoordinateMatrix rectangle(3, 5);
    require(rectangle.rows() == 3 && rectangle.cols() == 5 && rectangle.nnz() == 0,
            "Construction must preserve dimensions without creating entries");
    consistent(rectangle);
}

void sparse_dimensions()
{
    // COO stores entries, not rows*cols values. Such a shape is legal without
    // allocating a dense matrix or multiplying dimensions.
    const auto maximum = std::numeric_limits<std::size_t>::max();
    CoordinateMatrix matrix(maximum, maximum);
    require(matrix.nnz() == 0, "Large sparse shape must start empty");
    matrix.insert(maximum - 1, maximum - 1, 2.5);
    consistent(matrix);
    require(matrix.nnz() == 1 && matrix.row_indices()[0] == maximum - 1 &&
            matrix.col_indices()[0] == maximum - 1 && matrix.values()[0] == 2.5,
            "Large sparse coordinate was not preserved");
}

void insertion()
{
    CoordinateMatrix matrix(3, 5);
    matrix.insert(2, 4, -2.5);
    matrix.insert(0, 1, 3.0);
    matrix.insert(2, 0, 7.0);
    consistent(matrix);
    require(matrix.nnz() == 3, "Entry count must increase on insertion");
    require(matrix.row_indices() == std::vector<std::size_t>{2, 0, 2}, "Insertion reordered rows");
    require(matrix.col_indices() == std::vector<std::size_t>{4, 1, 0}, "Incorrect column storage");
    require(matrix.values() == std::vector<double>{-2.5, 3.0, 7.0}, "Incorrect value storage");
    // Row 1 deliberately has no entries.
    require(matrix.rows() == 3 && matrix.cols() == 5, "Insertion must not change dimensions");
}

void duplicates()
{
    CoordinateMatrix matrix(2, 3);
    matrix.insert(1, 2, 2.0);
    matrix.insert(0, 0, 8.0);
    matrix.insert(1, 2, 3.0);
    matrix.insert(1, 2, -5.0);
    consistent(matrix);
    require(matrix.nnz() == 4, "Insert must retain duplicate entries until explicit merging");
    require(matrix.row_indices() == std::vector<std::size_t>{1, 0, 1, 1} &&
            matrix.col_indices() == std::vector<std::size_t>{2, 0, 2, 2} &&
            matrix.values() == std::vector<double>{2.0, 8.0, 3.0, -5.0},
            "Duplicate entries lost their original coordinates or values");
}

void explicit_zeros()
{
    CoordinateMatrix matrix(2, 2);
    matrix.insert(1, 1, 7.0);
    const CoordinateMatrix saved(matrix);
    matrix.insert(0, 1, 0.0);
    matrix.insert(1, 0, -0.0);
    same(matrix, saved);
    out_of_range([&] { matrix.insert(2, 0, 0.0); });
    same(matrix, saved);
}

void bounds()
{
    const auto maximum = std::numeric_limits<std::size_t>::max();
    CoordinateMatrix matrix(2, 3);
    matrix.insert(1, 2, 4.0);
    const CoordinateMatrix saved(matrix);
    for (const auto& coordinate : {std::pair<std::size_t, std::size_t>{2, 0}, {0, 3},
                                   {2, 3}, {maximum, 0}, {0, maximum}}) {
        out_of_range([&] { matrix.insert(coordinate.first, coordinate.second, 9.0); });
        same(matrix, saved);
    }
    for (const auto& shape : {std::pair<std::size_t, std::size_t>{0, 0}, {0, 3}, {2, 0}}) {
        CoordinateMatrix empty(shape.first, shape.second);
        out_of_range([&] { empty.insert(0, 0, 1.0); });
        consistent(empty);
        require(empty.nnz() == 0 && empty.rows() == shape.first && empty.cols() == shape.second,
                "Rejected insertion changed an empty matrix");
    }
}

void copy_assignment()
{
    CoordinateMatrix source(2, 3);
    source.insert(1, 2, 2.5);
    CoordinateMatrix copy(source);
    same(copy, source);
    copy.insert(0, 1, 7.0);
    require(source.nnz() == 1 && copy.nnz() == 2, "Copy must own independent storage");
    CoordinateMatrix target(7, 9);
    target.insert(0, 0, -1.0);
    require(&(target = source) == &target, "Assignment must return its destination");
    same(target, source);
    const CoordinateMatrix& alias = target;
    target = alias;
    same(target, source);
    target.insert(0, 0, 3.0);
    require(source.nnz() == 1, "Assignment must own independent storage");
    target = CoordinateMatrix();
    same(target, CoordinateMatrix());
}

void growth()
{
    CoordinateMatrix matrix(7, 11);
    for (std::size_t i = 0; i < 2048; ++i) {
        matrix.insert(i % 7, (i * 3) % 11, static_cast<double>(i) - 0.5);
        consistent(matrix);
        require(matrix.nnz() == i + 1, "Incorrect count after repeated insertions");
    }
    for (std::size_t i = 0; i < matrix.nnz(); ++i) {
        require(matrix.row_indices()[i] == i % 7 && matrix.col_indices()[i] == (i * 3) % 11 &&
                matrix.values()[i] == static_cast<double>(i) - 0.5,
                "Growing arrays lost correspondence between coordinates and values");
    }
}

void canonical_empty()
{
    for (const auto& shape : {std::pair<std::size_t, std::size_t>{0, 0}, {0, 3}, {2, 0}, {3, 5}}) {
        CoordinateMatrix matrix(shape.first, shape.second);
        matrix.canonicalize();
        consistent(matrix);
        require(matrix.nnz() == 0 && matrix.rows() == shape.first && matrix.cols() == shape.second,
                "Canonicalization must preserve empty matrix shape");
    }
}

void canonical_sort()
{
    CoordinateMatrix matrix(4, 6);
    matrix.insert(3, 1, -7.0);
    matrix.insert(0, 5, 2.5);
    matrix.insert(1, 4, 9.0);
    matrix.insert(0, 1, -3.0);
    matrix.insert(1, 0, 6.0);
    matrix.canonicalize();
    consistent(matrix);
    require(matrix.rows() == 4 && matrix.cols() == 6, "Canonicalization changed dimensions");
    require(matrix.row_indices() == std::vector<std::size_t>{0, 0, 1, 1, 3} &&
            matrix.col_indices() == std::vector<std::size_t>{1, 5, 0, 4, 1} &&
            matrix.values() == std::vector<double>{-3.0, 2.5, 6.0, 9.0, -7.0},
            "Sort did not preserve coordinate/value correspondence");
}

void canonical_duplicates()
{
    CoordinateMatrix matrix(3, 5);
    matrix.insert(1, 2, 1.0);
    matrix.insert(0, 4, 5.0);
    matrix.insert(1, 2, 2.0);
    matrix.insert(2, 0, -4.0);
    matrix.insert(1, 2, 3.0);
    matrix.insert(0, 4, -2.0);
    matrix.canonicalize();
    consistent(matrix);
    require(matrix.row_indices() == std::vector<std::size_t>{0, 1, 2} &&
            matrix.col_indices() == std::vector<std::size_t>{4, 2, 0} &&
            matrix.values() == std::vector<double>{3.0, 6.0, -4.0},
            "Three or more duplicates must accumulate into a single entry");
    CoordinateMatrix ordered_sum(2, 2);
    ordered_sum.insert(1, 1, 1e16);
    ordered_sum.insert(1, 1, -1e16);
    ordered_sum.insert(1, 1, 1.0);
    ordered_sum.canonicalize();
    require(ordered_sum.nnz() == 1 && ordered_sum.values()[0] == 1.0,
            "Equal coordinates must accumulate in insertion order");
}

void canonical_cancellation()
{
    CoordinateMatrix matrix(3, 4);
    matrix.insert(1, 2, 4.0);
    matrix.insert(0, 1, 5.0);
    matrix.insert(1, 2, -4.0);
    matrix.insert(2, 3, 8.0);
    matrix.insert(0, 1, -5.0);
    matrix.insert(1, 2, 2.0);
    matrix.insert(2, 3, -8.0);
    matrix.canonicalize();
    consistent(matrix);
    require(matrix.nnz() == 1 && matrix.row_indices()[0] == 1 &&
            matrix.col_indices()[0] == 2 && matrix.values()[0] == 2.0,
            "Remove zero groups only after accumulating every entry");
    CoordinateMatrix cancelled(2, 3);
    cancelled.insert(1, 2, 3.0);
    cancelled.insert(1, 2, -3.0);
    cancelled.canonicalize();
    same(cancelled, CoordinateMatrix(2, 3));
}

void canonical_idempotence()
{
    CoordinateMatrix matrix(3, 4);
    matrix.insert(2, 3, 1.0);
    matrix.insert(0, 1, 3.0);
    matrix.insert(2, 3, 2.0);
    matrix.canonicalize();
    const CoordinateMatrix once(matrix);
    matrix.canonicalize();
    same(matrix, once);
    matrix.insert(0, 1, -3.0);
    matrix.insert(1, 2, 7.0);
    matrix.insert(2, 3, 4.0);
    matrix.canonicalize();
    CoordinateMatrix expected(3, 4);
    expected.insert(1, 2, 7.0);
    expected.insert(2, 3, 7.0);
    same(matrix, expected);
    matrix.canonicalize();
    same(matrix, expected);
}

void canonical_large_rows()
{
    if constexpr (std::numeric_limits<std::size_t>::digits > 53) {
        const std::size_t base = static_cast<std::size_t>(9007199254740992ULL);
        CoordinateMatrix matrix(base + 4, 2);
        matrix.insert(base + 1, 1, 2.0);
        matrix.insert(base, 1, 3.0);
        matrix.canonicalize();
        consistent(matrix);
        require(matrix.row_indices() == std::vector<std::size_t>{base, base + 1} &&
                matrix.col_indices() == std::vector<std::size_t>{1, 1} &&
                matrix.values() == std::vector<double>{3.0, 2.0},
                "Distinct large integer rows must not be rounded or merged");
    }
}

void canonical_large_cols()
{
    if constexpr (std::numeric_limits<std::size_t>::digits > 53) {
        const std::size_t base = static_cast<std::size_t>(9007199254740992ULL);
        CoordinateMatrix matrix(2, base + 4);
        matrix.insert(1, base + 1, 2.0);
        matrix.insert(1, base, 3.0);
        matrix.canonicalize();
        consistent(matrix);
        require(matrix.row_indices() == std::vector<std::size_t>{1, 1} &&
                matrix.col_indices() == std::vector<std::size_t>{base, base + 1} &&
                matrix.values() == std::vector<double>{3.0, 2.0},
                "Distinct large integer columns must not be rounded or merged");
    }
}

struct TestCase { const char* name; void (*run)(); };
const TestCase cases[] = {
    {"construction", construction}, {"sparse_dimensions", sparse_dimensions},
    {"insertion", insertion}, {"duplicates", duplicates}, {"explicit_zeros", explicit_zeros},
    {"bounds", bounds}, {"copy_assignment", copy_assignment}, {"growth", growth},
    {"canonical_empty", canonical_empty}, {"canonical_sort", canonical_sort},
    {"canonical_duplicates", canonical_duplicates}, {"canonical_cancellation", canonical_cancellation},
    {"canonical_idempotence", canonical_idempotence},
    {"canonical_large_rows", canonical_large_rows}, {"canonical_large_cols", canonical_large_cols}
};
}

int main(int argc, char* argv[])
{
    if (argc != 2) {
        std::cerr << "Usage: test_coordinate_matrix <case-name>\n";
        return 2;
    }
    for (const auto& test : cases) {
        if (test.name == std::string(argv[1])) {
            try {
                test.run();
                std::cout << "PASS: " << test.name << '\n';
                return 0;
            } catch (const std::exception& error) {
                std::cerr << "FAIL: " << test.name << ": " << error.what() << '\n';
                return 1;
            }
        }
    }
    std::cerr << "Unknown case: " << argv[1] << '\n';
    return 2;
}
