#include "linalg/vector.hpp"

#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

using minicae::Vector;

static_assert(std::is_same_v<decltype(std::declval<Vector &>()[0]), double &>);
static_assert(std::is_same_v<decltype(std::declval<const Vector &>()[0]), const double &>);
static_assert(noexcept(std::declval<const Vector &>().size()));

namespace {

    void require(bool condition, const std::string &message) {
        if (!condition) {
            throw std::runtime_error(message);
        }
    }

    void near(double actual, double expected) {
        const double tolerance = 1e-12 * std::max(1.0, std::abs(expected));
        require(std::isfinite(actual) && std::abs(actual - expected) <= tolerance,
                "Expected " + std::to_string(expected) + ", got " + std::to_string(actual));
    }

    void equal(const Vector &actual, std::initializer_list<double> expected) {
        require(actual.size() == expected.size(), "Unexpected vector size");
        std::size_t index = 0;
        for (double value : expected) {
            near(actual[index++], value);
        }
    }

    template <class Exception, class Function> void throws(Function function) {
        try {
            function();
        } catch (const Exception &) {
            return;
        }
        throw std::runtime_error("Expected exception was not thrown");
    }

    void construction() {
        require(Vector().size() == 0, "Default vector must be empty");
        require(Vector(std::size_t{0}).size() == 0, "Zero-length vector must be empty");
        equal(Vector(std::size_t{3}), {0.0, 0.0, 0.0});
        equal(Vector{1.5, -2.0, 0.0}, {1.5, -2.0, 0.0});
        // Parentheses select the length constructor; braces select the value list.
        equal(Vector{3.0}, {3.0});
    }

    void access() {
        Vector x(std::size_t{3});
        x[0] = 1.5;
        x[1] = -2.0;
        x[2] = 4.0;
        double &first = x[0];
        first += 0.5;
        const Vector &view = x;
        equal(view, {2.0, -2.0, 4.0});
    }

    void copy() {
        Vector source{1.0, 2.0};
        Vector copied(source);
        copied[0] = 9.0;
        equal(source, {1.0, 2.0});
        equal(copied, {9.0, 2.0});
        Vector empty;
        require(Vector(empty).size() == 0, "Empty copy must be empty");
    }

    void assignment() {
        Vector source{1.0, 2.0};
        Vector target(std::size_t{5});
        require(&(target = source) == &target, "Assignment must return the destination");
        target[1] = 7.0;
        equal(source, {1.0, 2.0});
        equal(target, {1.0, 7.0});
        const Vector &alias = target;
        target = alias;
        equal(target, {1.0, 7.0});
        target = Vector();
        require(target.size() == 0, "Assignment from empty must clear the destination");
        target = source;
        equal(target, {1.0, 2.0});
    }

    void arithmetic() {
        const Vector x{1.0, -2.0, 3.5};
        const Vector y{4.0, 2.0, -0.5};
        equal(x + y, {5.0, 0.0, 3.0});
        equal(x - y, {-3.0, -4.0, 4.0});
        equal(x - x, {0.0, 0.0, 0.0});
        equal(x, {1.0, -2.0, 3.5});
        equal(y, {4.0, 2.0, -0.5});
    }

    void scaling() {
        const Vector x{1.5, -2.0, 0.0};
        equal(2.0 * x, {3.0, -4.0, 0.0});
        equal(x * -2.0, {-3.0, 4.0, 0.0});
        equal(0.0 * x, {0.0, 0.0, 0.0});
        equal(x * 1.0, {1.5, -2.0, 0.0});
        equal(x, {1.5, -2.0, 0.0});
    }

    void inner_product() {
        near(minicae::dot(Vector{1.0, -2.0, 3.0}, Vector{4.0, 5.0, -6.0}), -24.0);
        near(minicae::dot(Vector{1.0, 0.0}, Vector{0.0, 1.0}), 0.0);
        near(minicae::dot(Vector{3.0, 4.0}, Vector{3.0, 4.0}), 25.0);
    }

    void norm() {
        near(minicae::norm2(Vector{3.0, -4.0}), 5.0);
        near(minicae::norm2(Vector{1.0, 1.0}), std::sqrt(2.0));
        near(minicae::norm2(Vector(std::size_t{3})), 0.0);
    }

    // Relative comparison without an absolute tolerance floor: returning zero
    // for a tiny, nonzero norm must fail this check.
    void positive_relative(double actual, double expected) {
        require(expected > 0.0 && std::isfinite(expected), "Invalid reference norm");
        require(actual > 0.0 && std::isfinite(actual), "Expected a positive finite norm");
        near(actual / expected, 1.0);
    }

    void norm_large() {
        positive_relative(minicae::norm2(Vector{1e200}), 1e200);
        positive_relative(minicae::norm2(Vector{3e200, -4e200}), 5e200);
        positive_relative(minicae::norm2(Vector{-4e200, 3e200}), 5e200);
        const double largest = std::numeric_limits<double>::max();
        positive_relative(minicae::norm2(Vector{-largest}), largest);
        // Infinity is correct when the final norm itself exceeds double's range.
        const double overflow = minicae::norm2(Vector{largest, largest});
        require(std::isinf(overflow) && overflow > 0.0, "Unrepresentable norm must be +infinity");
    }

    void norm_small() {
        positive_relative(minicae::norm2(Vector{1e-200}), 1e-200);
        positive_relative(minicae::norm2(Vector{3e-200, -4e-200}), 5e-200);
        positive_relative(minicae::norm2(Vector{-4e-200, 3e-200}), 5e-200);
        const double smallest_normal = std::numeric_limits<double>::min();
        positive_relative(minicae::norm2(Vector{-smallest_normal}), smallest_normal);
        if (std::numeric_limits<double>::has_denorm == std::denorm_present) {
            const double smallest = std::numeric_limits<double>::denorm_min();
            require(minicae::norm2(Vector{smallest}) == smallest,
                    "Single subnormal value must retain its magnitude");
        }
    }

    void norm_mixed_scales() {
        positive_relative(minicae::norm2(Vector{1e-200, 0.0, -1e200, -0.0}), 1e200);
        positive_relative(minicae::norm2(Vector{-1e200, 0.0, 1e-200}), 1e200);
        positive_relative(minicae::norm2(Vector{0.0, 3e-200, 0.0, -4e-200}), 5e-200);
        const double zero = minicae::norm2(Vector{-0.0, 0.0});
        require(zero == 0.0 && !std::signbit(zero), "Zero norm must be positive zero");
    }

    void norm_nan() {
        const double nan = std::numeric_limits<double>::quiet_NaN();
        const double inf = std::numeric_limits<double>::infinity();
        // Contract of this implementation: any NaN propagates, even with infinity.
        require(std::isnan(minicae::norm2(Vector{nan})), "NaN must propagate");
        require(std::isnan(minicae::norm2(Vector{1.0, nan, 2.0})), "Interior NaN must propagate");
        require(std::isnan(minicae::norm2(Vector{nan, inf})), "NaN before infinity must propagate");
        require(std::isnan(minicae::norm2(Vector{inf, nan})), "NaN after infinity must propagate");
    }

    void norm_infinity() {
        const double inf = std::numeric_limits<double>::infinity();
        for (const Vector& x : {Vector{inf}, Vector{-inf}, Vector{inf, -inf},
                                Vector{0.0, 2.0, inf}, Vector{-inf, 3.0, 0.0}}) {
            const double actual = minicae::norm2(x);
            require(std::isinf(actual) && actual > 0.0, "Infinite magnitude must yield +infinity");
        }
    }

    void negation() {
        Vector x{1.5, -2.0, 0.0};
        equal(-x, {-1.5, 2.0, 0.0});
        equal(x, {1.5, -2.0, 0.0});
        minicae::negate(x);
        equal(x, {-1.5, 2.0, 0.0});
        minicae::negate(x);
        equal(x, {1.5, -2.0, 0.0});
    }

    void axpy() {
        const Vector x{1.0, -2.0, 3.0};
        Vector y{4.0, 5.0, -6.0};
        minicae::axpy(2.0, x, y);
        equal(y, {6.0, 1.0, 0.0});
        minicae::axpy(0.0, x, y);
        equal(y, {6.0, 1.0, 0.0});
        minicae::axpy(-1.0, x, y);
        equal(y, {5.0, 3.0, -3.0});
        equal(x, {1.0, -2.0, 3.0});
        minicae::axpy(2.0, y, y);
        equal(y, {15.0, 9.0, -9.0});
    }

    void empty_operations() {
        const Vector x;
        Vector y;
        equal(x + y, {});
        equal(x - y, {});
        equal(2.0 * x, {});
        equal(x * 2.0, {});
        equal(-x, {});
        near(minicae::dot(x, y), 0.0);
        near(minicae::norm2(x), 0.0);
        minicae::negate(y);
        minicae::axpy(2.0, x, y);
        require(y.size() == 0, "Empty operations must preserve size");
    }

    void mismatched_sizes() {
        // Exercise both size orders, including empty/nonempty combinations.
        for (std::size_t nx : {std::size_t{0}, std::size_t{1}, std::size_t{3}}) {
            for (std::size_t ny : {std::size_t{0}, std::size_t{1}, std::size_t{3}}) {
                if (nx == ny) {
                    continue;
                }
                const Vector x(nx);
                Vector y(ny);
                throws<std::invalid_argument>([&] { (void)(x + y); });
                throws<std::invalid_argument>([&] { (void)(x - y); });
                throws<std::invalid_argument>([&] { (void)minicae::dot(x, y); });
                throws<std::invalid_argument>([&] { minicae::axpy(0.0, x, y); });
                require(y.size() == ny, "Rejected AXPY must preserve destination size");
                for (std::size_t i = 0; i < ny; ++i) {
                    near(y[i], 0.0);
                }
            }
        }
        const Vector x{1.0, 2.0};
        Vector y{7.0};
        throws<std::invalid_argument>([&] { minicae::axpy(2.0, x, y); });
        equal(y, {7.0});
    }

    // Both mutable and const access must reject invalid indices with std::out_of_range.
    void bounds_mutable() {
        Vector x{1.0, 2.0};
        throws<std::out_of_range>([&] { (void)x[x.size()]; });
        Vector empty;
        throws<std::out_of_range>([&] { (void)empty[0]; });
    }

    void bounds_const() {
        const Vector x{1.0, 2.0};
        throws<std::out_of_range>([&] { (void)x[x.size()]; });
        const Vector empty;
        throws<std::out_of_range>([&] { (void)empty[0]; });
    }

    struct TestCase {
        const char *name;
        void (*run)();
    };

    const TestCase cases[] = {{"construction", construction},
                              {"access", access},
                              {"copy", copy},
                              {"assignment", assignment},
                              {"arithmetic", arithmetic},
                              {"scaling", scaling},
                              {"dot", inner_product},
                              {"norm2", norm},
                              {"norm_large", norm_large},
                              {"norm_small", norm_small},
                              {"norm_mixed_scales", norm_mixed_scales},
                              {"norm_nan", norm_nan},
                              {"norm_infinity", norm_infinity},
                              {"negation", negation},
                              {"axpy", axpy},
                              {"empty_operations", empty_operations},
                              {"mismatched_sizes", mismatched_sizes},
                              {"bounds_mutable", bounds_mutable},
                              {"bounds_const", bounds_const}};

} // namespace

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: test_vector <case-name>\n";
        return 2;
    }
    for (const auto &test : cases) {
        if (test.name == std::string(argv[1])) {
            try {
                test.run();
                std::cout << "PASS: " << test.name << '\n';
                return 0;
            } catch (const std::exception &error) {
                std::cerr << "FAIL: " << test.name << ": " << error.what() << '\n';
                return 1;
            }
        }
    }
    std::cerr << "Unknown case: " << argv[1] << '\n';
    return 2;
}
