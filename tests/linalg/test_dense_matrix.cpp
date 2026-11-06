#include "linalg/dense_matrix.hpp"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

using minicae::DenseMatrix;

static_assert(std::is_same_v<decltype(std::declval<DenseMatrix&>()(0, 0)), double&>);
static_assert(std::is_same_v<decltype(std::declval<const DenseMatrix&>()(0, 0)), const double&>);
static_assert(noexcept(std::declval<const DenseMatrix&>().rows()));
static_assert(noexcept(std::declval<const DenseMatrix&>().cols()));

namespace {

void require(bool condition, const char* message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

template<class Exception, class Function>
void throws(Function function)
{
    try {
        function();
    } catch (const Exception&) {
        return;
    }
    throw std::runtime_error("Expected exception was not thrown");
}

void construction()
{
    const DenseMatrix empty;
    require(empty.rows() == 0 && empty.cols() == 0, "Default shape must be 0 x 0");
    const DenseMatrix matrix(2, 3);
    require(matrix.rows() == 2 && matrix.cols() == 3, "Rectangular shape not preserved");
    for (std::size_t i = 0; i < matrix.rows(); ++i) {
        for (std::size_t j = 0; j < matrix.cols(); ++j) {
            require(matrix(i, j) == 0.0, "Default elements must be zero");
        }
    }
}

void initial_value()
{
    for (double value : {-2.5, 0.0, 3.25}) {
        const DenseMatrix matrix(3, 2, value);
        for (std::size_t i = 0; i < matrix.rows(); ++i) {
            for (std::size_t j = 0; j < matrix.cols(); ++j) {
                require(matrix(i, j) == value, "Initial value not applied to every element");
            }
        }
    }
}

void access()
{
    // Both wide and tall shapes catch incorrect flattening strides.
    for (const auto& shape : {std::pair<std::size_t, std::size_t>{2, 3}, {3, 2}, {1, 4}, {4, 1}}) {
        DenseMatrix matrix(shape.first, shape.second);
        for (std::size_t i = 0; i < matrix.rows(); ++i) {
            for (std::size_t j = 0; j < matrix.cols(); ++j) {
                matrix(i, j) = static_cast<double>(10 * i + j) + 0.25;
            }
        }
        for (std::size_t i = 0; i < matrix.rows(); ++i) {
            for (std::size_t j = 0; j < matrix.cols(); ++j) {
                require(matrix(i, j) == static_cast<double>(10 * i + j) + 0.25,
                        "Element writes alias or use an incorrect index");
            }
        }
        double& first = matrix(0, 0);
        first = -7.5;
        require(matrix(0, 0) == -7.5, "Mutable access must return a reference");
    }
}

void const_access()
{
    DenseMatrix matrix(2, 3, 1.5);
    matrix(1, 2) = -8.0;
    const DenseMatrix& view = matrix;
    require(view(0, 0) == 1.5 && view(1, 2) == -8.0, "Const access returned wrong value");
}

void bounds_mutable()
{
    DenseMatrix matrix(2, 3);
    const auto maximum = std::numeric_limits<std::size_t>::max();
    throws<std::out_of_range>([&] { (void)matrix(2, 0); });
    // (0, 3) flattens to an in-range position, but the column is invalid.
    throws<std::out_of_range>([&] { (void)matrix(0, 3); });
    throws<std::out_of_range>([&] { (void)matrix(2, 3); });
    throws<std::out_of_range>([&] { (void)matrix(maximum, 0); });
    throws<std::out_of_range>([&] { (void)matrix(0, maximum); });
}

void bounds_const()
{
    const DenseMatrix matrix(2, 3);
    const auto maximum = std::numeric_limits<std::size_t>::max();
    throws<std::out_of_range>([&] { (void)matrix(2, 0); });
    throws<std::out_of_range>([&] { (void)matrix(0, 3); });
    throws<std::out_of_range>([&] { (void)matrix(2, 3); });
    throws<std::out_of_range>([&] { (void)matrix(maximum, 0); });
    throws<std::out_of_range>([&] { (void)matrix(0, maximum); });
}

void empty_shapes()
{
    const auto maximum = std::numeric_limits<std::size_t>::max();
    for (const auto& shape : {std::pair<std::size_t, std::size_t>{0, 0}, {0, 3}, {4, 0},
                              {0, maximum}, {maximum, 0}}) {
        DenseMatrix matrix(shape.first, shape.second);
        require(matrix.rows() == shape.first && matrix.cols() == shape.second,
                "Zero-sized matrix must preserve both dimensions");
        throws<std::out_of_range>([&] { (void)matrix(0, 0); });
        const DenseMatrix& view = matrix;
        throws<std::out_of_range>([&] { (void)view(0, 0); });
    }
}

void copy_assignment()
{
    DenseMatrix source(2, 3, 2.5);
    source(1, 2) = -4.0;
    DenseMatrix copy(source);
    copy(1, 2) = 9.0;
    require(source(1, 2) == -4.0, "Copy must own independent elements");
    DenseMatrix target(4, 1);
    require(&(target = source) == &target, "Assignment must return its destination");
    require(target.rows() == 2 && target.cols() == 3, "Assignment must copy shape");
    require(target(0, 0) == 2.5 && target(1, 2) == -4.0, "Assignment must copy values");
    target(0, 0) = 7.0;
    require(source(0, 0) == 2.5, "Assignment must own independent elements");
    const DenseMatrix& alias = target;
    target = alias;
    require(target(0, 0) == 7.0 && target(1, 2) == -4.0, "Self-assignment lost data");
    target = DenseMatrix();
    require(target.rows() == 0 && target.cols() == 0, "Empty assignment must reset shape");
}

void size_overflow()
{
    const auto maximum = std::numeric_limits<std::size_t>::max();
    // Each unchecked product wraps to 0 or 2. No huge allocation is attempted
    // even with the current buggy constructor. All cases must throw length_error.
    for (const auto& shape : {
             std::pair<std::size_t, std::size_t>{maximum / 2 + 1, 2},
             {2, maximum / 2 + 1}, {maximum / 2 + 2, 2}, {2, maximum / 2 + 2}}) {
        throws<std::length_error>([&] { (void)DenseMatrix(shape.first, shape.second); });
    }
}

DenseMatrix sample_matrix()
{
    DenseMatrix matrix(2, 3);
    matrix(0, 0) = 1.0; matrix(0, 1) = 2.0; matrix(0, 2) = 3.0;
    matrix(1, 0) = -4.0; matrix(1, 1) = 5.0; matrix(1, 2) = 6.0;
    return matrix;
}

void same_matrix(const DenseMatrix& actual, const DenseMatrix& expected)
{
    require(actual.rows() == expected.rows() && actual.cols() == expected.cols(),
            "Unexpected matrix shape");
    for (std::size_t i = 0; i < actual.rows(); ++i) {
        for (std::size_t j = 0; j < actual.cols(); ++j) {
            require(actual(i, j) == expected(i, j), "Unexpected matrix value");
        }
    }
}

void transpose_rectangular()
{
    const DenseMatrix original = sample_matrix();
    DenseMatrix matrix(original);
    DenseMatrix transposed = minicae::transpose(matrix);
    require(transposed.rows() == 3 && transposed.cols() == 2, "Transpose must swap shape");
    require(transposed(0, 0) == 1.0 && transposed(0, 1) == -4.0 &&
            transposed(1, 0) == 2.0 && transposed(1, 1) == 5.0 &&
            transposed(2, 0) == 3.0 && transposed(2, 1) == 6.0,
            "Rectangular transpose values are incorrect");
    same_matrix(matrix, original);
    same_matrix(minicae::transpose(transposed), original);
    transposed(0, 0) = 99.0;
    same_matrix(matrix, original);
    DenseMatrix row(1, 3, 2.5);
    const DenseMatrix column = minicae::transpose(row);
    require(column.rows() == 3 && column.cols() == 1 && column(2, 0) == 2.5,
            "Single-row transpose is incorrect");
    same_matrix(minicae::transpose(column), row);
}

void transpose_empty()
{
    for (const auto& shape : {std::pair<std::size_t, std::size_t>{0, 0}, {0, 3}, {4, 0}}) {
        const DenseMatrix matrix(shape.first, shape.second);
        const DenseMatrix result = minicae::transpose(matrix);
        require(result.rows() == shape.second && result.cols() == shape.first,
                "Empty transpose must swap and preserve dimensions");
        same_matrix(minicae::transpose(result), matrix);
    }
}

void matvec_rectangular()
{
    const DenseMatrix matrix = sample_matrix();
    const DenseMatrix original(matrix);
    const minicae::Vector x{2.0, -1.0, 0.5};
    const minicae::Vector result = matrix * x;
    require(result.size() == 2, "Product must have one element per matrix row");
    require(result[0] == 1.5 && result[1] == -10.0, "Incorrect hand-calculated product");
    same_matrix(matrix, original);
    require(x[0] == 2.0 && x[1] == -1.0 && x[2] == 0.5, "Product modified its input vector");
    const minicae::Vector tall_result = minicae::transpose(matrix) * minicae::Vector{2.0, -1.0};
    require(tall_result.size() == 3 && tall_result[0] == 6.0 &&
            tall_result[1] == -1.0 && tall_result[2] == 0.0, "Incorrect tall-matrix product");
}

void matvec_special()
{
    DenseMatrix identity(3, 3);
    for (std::size_t i = 0; i < 3; ++i) {
        identity(i, i) = 1.0;
    }
    const minicae::Vector x{1.5, -2.0, 0.0};
    const minicae::Vector product = identity * x;
    for (std::size_t i = 0; i < x.size(); ++i) {
        require(product[i] == x[i], "Identity product must preserve the vector");
    }
    const minicae::Vector zero = DenseMatrix(2, 3) * x;
    require(zero.size() == 2 && zero[0] == 0.0 && zero[1] == 0.0, "Zero matrix product failed");
    const minicae::Vector zero_input = sample_matrix() * minicae::Vector(std::size_t{3});
    require(zero_input[0] == 0.0 && zero_input[1] == 0.0, "Zero vector product failed");
    const minicae::Vector column = DenseMatrix(3, 1, -2.0) * minicae::Vector{1.5};
    require(column.size() == 3 && column[0] == -3.0 && column[2] == -3.0,
            "Single-column product failed");
}

void matvec_empty()
{
    require((DenseMatrix() * minicae::Vector()).size() == 0, "0 x 0 product must be empty");
    require((DenseMatrix(0, 3) * minicae::Vector{1.0, 2.0, 3.0}).size() == 0,
            "0 x n product must be empty");
    const minicae::Vector result = DenseMatrix(4, 0) * minicae::Vector();
    require(result.size() == 4, "m x 0 product must preserve row count");
    for (std::size_t i = 0; i < result.size(); ++i) {
        require(result[i] == 0.0, "m x 0 product must be zero");
    }
}

void matvec_mismatch()
{
    const DenseMatrix matrix = sample_matrix();
    for (std::size_t size : {std::size_t{0}, std::size_t{2}, std::size_t{4}}) {
        throws<std::invalid_argument>([&] { (void)(matrix * minicae::Vector(size)); });
    }
    throws<std::invalid_argument>([] { (void)(DenseMatrix(0, 3) * minicae::Vector()); });
    throws<std::invalid_argument>([] { (void)(DenseMatrix(4, 0) * minicae::Vector{1.0}); });
}

void matmul_rectangular()
{
    const DenseMatrix a = sample_matrix();
    DenseMatrix b(3, 2);
    b(0, 0) = 7.0; b(0, 1) = 8.0;
    b(1, 0) = 9.0; b(1, 1) = 10.0;
    b(2, 0) = 11.0; b(2, 1) = 12.0;
    const DenseMatrix saved_b(b);
    const DenseMatrix result = a * b;
    require(result.rows() == 2 && result.cols() == 2, "Incorrect rectangular product shape");
    require(result(0, 0) == 58.0 && result(0, 1) == 64.0 &&
            result(1, 0) == 83.0 && result(1, 1) == 90.0, "Incorrect hand-calculated product");
    same_matrix(a, sample_matrix());
    same_matrix(b, saved_b);
    const DenseMatrix reversed = b * a;
    require(reversed.rows() == 3 && reversed.cols() == 3 && reversed(0, 0) == -25.0 &&
            reversed(2, 2) == 105.0, "Incorrect reversed rectangular product");
    DenseMatrix column(3, 1);
    column(0, 0) = 1.0; column(1, 0) = 2.0; column(2, 0) = -1.0;
    DenseMatrix row(1, 2);
    row(0, 0) = 3.0; row(0, 1) = -2.0;
    const DenseMatrix outer = column * row;
    require(outer.rows() == 3 && outer.cols() == 2 && outer(1, 0) == 6.0 &&
            outer(2, 1) == 2.0, "Incorrect outer product");
    const DenseMatrix inner = minicae::transpose(column) * column;
    require(inner.rows() == 1 && inner.cols() == 1 && inner(0, 0) == 6.0,
            "Incorrect row-by-column product");
}

void matmul_identity_zero()
{
    const DenseMatrix a = sample_matrix();
    DenseMatrix left_identity(2, 2), right_identity(3, 3);
    for (std::size_t i = 0; i < 2; ++i) { left_identity(i, i) = 1.0; }
    for (std::size_t i = 0; i < 3; ++i) { right_identity(i, i) = 1.0; }
    same_matrix(left_identity * a, a);
    same_matrix(a * right_identity, a);
    same_matrix(DenseMatrix(4, 2) * a, DenseMatrix(4, 3));
    same_matrix(a * DenseMatrix(3, 4), DenseMatrix(2, 4));
}

void matmul_self()
{
    DenseMatrix a(2, 2);
    a(0, 0) = 1.0; a(0, 1) = 2.0; a(1, 1) = 1.0;
    DenseMatrix result = a * a;
    require(result(0, 0) == 1.0 && result(0, 1) == 4.0 &&
            result(1, 0) == 0.0 && result(1, 1) == 1.0, "Incorrect self product");
    require(a(0, 1) == 2.0, "Self product must preserve the input");
    result(0, 1) = 99.0;
    require(a(0, 1) == 2.0, "Product must own independent storage");
}

void matmul_empty()
{
    same_matrix(DenseMatrix() * DenseMatrix(), DenseMatrix());
    same_matrix(DenseMatrix(0, 3) * DenseMatrix(3, 2), DenseMatrix(0, 2));
    same_matrix(DenseMatrix(2, 3) * DenseMatrix(3, 0), DenseMatrix(2, 0));
    same_matrix(DenseMatrix(2, 0) * DenseMatrix(0, 4), DenseMatrix(2, 4));
}

void matmul_mismatch()
{
    const DenseMatrix a = sample_matrix();
    for (std::size_t rows : {std::size_t{0}, std::size_t{2}, std::size_t{4}}) {
        throws<std::invalid_argument>([&] { (void)(a * DenseMatrix(rows, 2)); });
    }
    throws<std::invalid_argument>([] { (void)(DenseMatrix(0, 3) * DenseMatrix(2, 0)); });
    throws<std::invalid_argument>([] { (void)(DenseMatrix(2, 0) * DenseMatrix(1, 3)); });
}

void matmul_result_overflow()
{
    const auto maximum = std::numeric_limits<std::size_t>::max();
    const DenseMatrix a(maximum / 2 + 1, 0), b(0, 2);
    throws<std::length_error>([&] { (void)(a * b); });
}

struct TestCase {
    const char* name;
    void (*run)();
};

const TestCase cases[] = {
    {"construction", construction}, {"initial_value", initial_value},
    {"access", access}, {"const_access", const_access},
    {"bounds_mutable", bounds_mutable}, {"bounds_const", bounds_const},
    {"empty_shapes", empty_shapes}, {"copy_assignment", copy_assignment},
    {"size_overflow", size_overflow},
    {"transpose_rectangular", transpose_rectangular}, {"transpose_empty", transpose_empty},
    {"matvec_rectangular", matvec_rectangular}, {"matvec_special", matvec_special},
    {"matvec_empty", matvec_empty}, {"matvec_mismatch", matvec_mismatch},
    {"matmul_rectangular", matmul_rectangular}, {"matmul_identity_zero", matmul_identity_zero},
    {"matmul_self", matmul_self}, {"matmul_empty", matmul_empty},
    {"matmul_mismatch", matmul_mismatch}, {"matmul_result_overflow", matmul_result_overflow}
};

} // namespace

int main(int argc, char* argv[])
{
    if (argc != 2) {
        std::cerr << "Usage: test_dense_matrix <case-name>\n";
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
