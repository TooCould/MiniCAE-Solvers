#include "linalg/compressed_sparse_row.hpp"
#include "linalg/coordinate_matrix.hpp"
#include "linalg/dense_matrix.hpp"

#include <algorithm>
#include <utility>
#include <cmath>
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>
#include <string>

using namespace minicae;
namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
template<class Exception, class Function>
void throws(Function function) {
    try { function(); } catch (const Exception&) { return; }
    throw std::runtime_error("Expected exception not thrown");
}
void construction() {
    CompressedSparseRow{}.validate();
    for (auto shape : {std::pair<std::size_t,std::size_t>{0,4}, {4,0}, {3,5}}) {
        const CompressedSparseRow a(shape.first, shape.second);
        a.validate();
        require(a.rows()==shape.first && a.cols()==shape.second && a.nnz()==0,
                "Wrong shape/nnz");
        require(a.row_ptr()==std::vector<std::size_t>(shape.first+1,0), "Wrong zero pointers");
    }
    const auto max = std::numeric_limits<std::size_t>::max();
    throws<std::length_error>([&] { CompressedSparseRow a(max,0); });
    throws<std::length_error>([&] { CompressedSparseRow a(CoordinateMatrix(max,0)); });
    throws<std::invalid_argument>([&] { CompressedSparseRow a(max,0,{}, {}, {}); });
}
void conversion() {
    CoordinateMatrix coo(5,4);
    coo.insert(3,2,5); coo.insert(1,3,2); coo.insert(1,0,4);
    coo.insert(1,3,3); coo.insert(2,1,7); coo.insert(2,1,-7);
    const auto old_rows=coo.row_indices();
    const auto old_cols=coo.col_indices();
    const auto old_values=coo.values();
    const CompressedSparseRow a(coo);
    a.validate();
    require(a.rows()==5 && a.cols()==4, "Conversion shape");
    require(a.row_ptr()==std::vector<std::size_t>{0,0,2,2,3,3}, "Conversion pointers");
    require(a.col_indices()==std::vector<std::size_t>{0,3,2}, "Conversion columns");
    require(a.values()==std::vector<double>{4,5,5}, "Conversion values");
    require(coo.row_indices()==old_rows && coo.col_indices()==old_cols &&
            coo.values()==old_values, "Conversion changed input");
    CoordinateMatrix cancellation(2,3);
    cancellation.insert(1,1,2); cancellation.insert(1,1,-2);
    const CompressedSparseRow zero(cancellation);
    zero.validate();
    require(zero.nnz()==0 && zero.row_ptr()==std::vector<std::size_t>{0,0,0}, "Cancellation");
    CompressedSparseRow(CoordinateMatrix(0,4)).validate();
    CompressedSparseRow(CoordinateMatrix(4,0)).validate();
}
void invalid_structure() {
    auto reject=[](std::size_t r, std::size_t c, std::vector<std::size_t> ptr,
                   std::vector<std::size_t> col, std::vector<double> val) {
        throws<std::invalid_argument>([&] { CompressedSparseRow a(r,c,ptr,col,val); });
    };
    reject(1,3,{}, {}, {});
    reject(1,3,{0}, {}, {});
    reject(1,3,{1,1}, {0}, {1});
    reject(1,3,{0,0}, {0}, {1});
    reject(1,3,{0,1}, {}, {1});
    reject(2,3,{0,3,1}, {0}, {1});
    reject(3,3,{0,2,1,2}, {0,1}, {1,2});
    reject(1,3,{0,1}, {3}, {1});
    reject(1,3,{0,2}, {0,3}, {1,2});
    reject(1,3,{0,2}, {1,1}, {1,2});
    reject(1,3,{0,2}, {2,0}, {1,2});
    reject(1,3,{0,1}, {0}, {0});
    reject(1,3,{0,2}, {0,2}, {1,-0.0});
}
void matvec() {
    const CompressedSparseRow a(4,3,{0,2,2,3,4},{0,2,1,0},{2,-1,3,-2});
    const Vector x{4,5,6};
    const auto old_values=a.values();
    const Vector y=a*x;
    require(y.size()==4 && y[0]==2 && y[1]==0 && y[2]==15 && y[3]==-8,
            "Hand-calculated SpMV mismatch");
    require(x[0]==4 && x[1]==5 && x[2]==6 && a.values()==old_values, "SpMV modified input");
    throws<std::invalid_argument>([&] { (void)(a*Vector(2)); });
    require((CompressedSparseRow(0,3)*Vector(3)).size()==0, "Zero rows");
    const Vector zero=CompressedSparseRow(3,0)*Vector{};
    require(zero.size()==3 && zero[0]==0 && zero[1]==0 && zero[2]==0, "Zero columns");
    require((CompressedSparseRow{}*Vector{}).size()==0, "Empty SpMV");
    throws<std::invalid_argument>([] { (void)(CompressedSparseRow(0,3)*Vector{}); });
}
void dense_reference() {
    std::mt19937 generator(20261110);
    std::uniform_int_distribution<int> value(-4,4);
    for (std::size_t rows : {0,1,5,17}) {
        for (std::size_t cols : {0,1,3,11}) {
            CoordinateMatrix coo(rows,cols);
            DenseMatrix dense(rows,cols);
            for (int k=0; k<200 && rows!=0 && cols!=0; ++k) {
                const auto r=generator()%rows, c=generator()%cols;
                const double v=value(generator);
                coo.insert(r,c,v); dense(r,c)+=v;
            }
            const CompressedSparseRow a(coo);
            a.validate();
            Vector x(cols);
            for (std::size_t i=0; i<cols; ++i) x[i]=value(generator);
            const Vector actual=a*x, expected=dense*x;
            require(actual.size()==expected.size(), "Result shape mismatch");
            for (std::size_t i=0; i<rows; ++i)
                require(actual[i]==expected[i], "Dense/CSR mismatch");
        }
    }
}
void diagonal_cases() {
    const CompressedSparseRow tall(4,2,{0,1,1,2,2},{0,1},{2,7});
    const Vector d=diagonal(tall);
    require(d.size()==2 && d[0]==2 && d[1]==0, "Tall diagonal/missing entry");
    const CompressedSparseRow wide(2,4,{0,2,3},{0,3,1},{-2,7,0.25});
    const Vector w=diagonal(wide);
    require(w.size()==2 && w[0]==-2 && w[1]==0.25, "Wide diagonal");
    require(diagonal(CompressedSparseRow{}).size()==0, "Empty diagonal");
    require(diagonal(CompressedSparseRow(0,3)).size()==0, "Zero-row diagonal");
    require(diagonal(CompressedSparseRow(3,0)).size()==0, "Zero-column diagonal");
    const Vector zero=diagonal(CompressedSparseRow(3,3));
    require(zero.size()==3 && zero[0]==0 && zero[1]==0 && zero[2]==0, "Zero diagonal");
}
void norm_inf_cases() {
    // Signed row sum is -0.25; absolute row sum is 0.75.
    require(norm_inf(CompressedSparseRow(1,2,{0,2},{0,1},{-0.5,0.25}))==0.75,
            "Fractional absolute row sum");
    const CompressedSparseRow a(4,3,{0,0,2,3,5},{0,2,1,0,2},{2,-3,-1,-4,2});
    require(norm_inf(a)==6, "Maximum row sum with empty row");
    require(norm_inf(CompressedSparseRow{})==0, "Empty norm");
    require(norm_inf(CompressedSparseRow(0,3))==0, "Zero-row norm");
    require(norm_inf(CompressedSparseRow(3,0))==0, "Zero-column norm");
    require(norm_inf(CompressedSparseRow(3,4))==0, "Zero matrix norm");
}
void symmetry_cases() {
    const CompressedSparseRow empty;
    require(is_symmetric(empty), "Empty symmetry");
    require(is_symmetric(CompressedSparseRow(3,3)), "Zero symmetry");
    require(!is_symmetric(CompressedSparseRow(2,3)), "Rectangular symmetry");
    const CompressedSparseRow identity(2,2,{0,1,2},{0,1},{1,1});
    require(is_symmetric(identity), "Exact symmetry with default tolerance");
    const CompressedSparseRow bottom(4,4,{0,0,0,1,1},{3},{2});
    require(!is_symmetric(bottom,0.01), "Unmatched entry in lower rows");
    const CompressedSparseRow missing(2,2,{0,1,1},{1},{0.25});
    require(is_symmetric(missing,0.25), "Missing counterpart at tolerance boundary");
    require(!is_symmetric(missing,0.125), "Missing counterpart beyond tolerance");
    const CompressedSparseRow boundary(2,2,{0,1,2},{1,0},{1,1.5});
    require(is_symmetric(boundary,0.5), "Difference equals tolerance");
    require(!is_symmetric(boundary,0.25), "Difference beyond tolerance");
    const CompressedSparseRow opposite(2,2,{0,1,2},{1,0},{0.75,-0.75});
    require(!is_symmetric(opposite,1), "Existing unequal counterpart treated as missing");
    const double inf=std::numeric_limits<double>::infinity();
    const double nan=std::numeric_limits<double>::quiet_NaN();
    for(double bad : {-1.0,inf,-inf,nan}) {
        throws<std::invalid_argument>([&] { (void)is_symmetric(empty,bad); });
        throws<std::invalid_argument>([&] { (void)is_symmetric(identity,bad); });
    }
    for(double bad : {inf,-inf,nan}) {
        const CompressedSparseRow nonfinite(3,3,{0,0,0,1},{2},{bad});
        require(!is_symmetric(nonfinite), "Nonfinite value in last row");
    }
}
void basic_ops_reference() {
    std::mt19937 generator(20261110);
    for(int trial=0; trial<300; ++trial) {
        const std::size_t rows=generator()%9;
        const std::size_t cols=trial%3==0 ? generator()%9 : rows;
        DenseMatrix dense(rows,cols);
        CoordinateMatrix coo(rows,cols);
        for(std::size_t i=0; i<rows; ++i) {
            for(std::size_t j=0; j<cols; ++j) {
                double value=(static_cast<int>(generator()%9)-4)*0.25;
                if(trial%4==0 && rows==cols && j<i) value=dense(j,i);
                dense(i,j)=value;
                coo.insert(i,j,value);
            }
        }
        const CompressedSparseRow a(coo);
        const auto old_ptr=a.row_ptr();
        const auto old_cols=a.col_indices();
        const auto old_values=a.values();
        const Vector d=diagonal(a);
        require(d.size()==std::min(rows,cols), "Reference diagonal length");
        double expected_norm=0;
        for(std::size_t i=0; i<rows; ++i) {
            double sum=0;
            for(std::size_t j=0; j<cols; ++j) sum+=std::abs(dense(i,j));
            expected_norm=std::max(expected_norm,sum);
            if(i<cols) require(d[i]==dense(i,i), "Reference diagonal mismatch");
        }
        require(norm_inf(a)==expected_norm, "Reference norm mismatch");
        for(double atol : {0.0,0.25,0.5,1.0,2.0}) {
            bool expected=rows==cols;
            if(expected) {
                for(std::size_t i=0; i<rows; ++i)
                    for(std::size_t j=0; j<cols; ++j)
                        if(std::abs(dense(i,j)-dense(j,i))>atol) expected=false;
            }
            require(is_symmetric(a,atol)==expected, "Reference symmetry mismatch");
        }
        require(a.row_ptr()==old_ptr && a.col_indices()==old_cols && a.values()==old_values,
                "Basic operations changed input");
    }
}

void storage_scaling() {
    // Vary row count and nnz independently; never allocate rows * cols storage.
    auto check=[](std::size_t rows, std::size_t cols, std::size_t count) {
        CoordinateMatrix coo(rows,cols);
        for(std::size_t k=0; k<count; ++k) coo.insert(k/cols,k%cols,1.0);
        const CompressedSparseRow a(coo);
        a.validate();
        require(a.nnz()==count, "Scaling nnz mismatch");
        require(a.row_ptr().size()==rows+1, "Row pointer storage must equal rows+1");
        require(a.col_indices().size()==count && a.values().size()==count,
                "Entry storage must equal nnz");
        const std::size_t payload=(rows+1)*sizeof(std::size_t)+
                                 count*(sizeof(std::size_t)+sizeof(double));
        const std::size_t capacity_bytes=a.row_ptr().capacity()*sizeof(std::size_t)+
            a.col_indices().capacity()*sizeof(std::size_t)+a.values().capacity()*sizeof(double);
        std::cout << "rows=" << rows << " cols=" << cols << " nnz=" << count
                  << " payload_bytes=" << payload << " capacity_bytes=" << capacity_bytes << '\n';
    };
    for(std::size_t rows : {10,1000,100000}) check(rows,16,8);
    for(std::size_t count : {64,1024,16384}) check(1001,32,count);
    // Column dimension alone must not allocate a dense row or rows * cols array.
    for(std::size_t cols : {16,1000000}) check(3,cols,3);
}
void zero_based_indices() {
    CoordinateMatrix coo(3,4);
    coo.insert(0,0,2); coo.insert(0,3,-1); coo.insert(2,3,5);
    throws<std::out_of_range>([&] { coo.insert(3,0,1); });
    throws<std::out_of_range>([&] { coo.insert(0,4,1); });
    const CompressedSparseRow a(coo);
    require(a.row_ptr()==std::vector<std::size_t>{0,2,2,3}, "Zero-based half-open row offsets");
    require(a.col_indices()==std::vector<std::size_t>{0,3,3}, "First/last column indices");
    const Vector y=a*Vector{3,0,0,2};
    require(y.size()==3 && y[0]==4 && y[1]==0 && y[2]==10, "Boundary index SpMV");
    DenseMatrix dense(3,4);
    dense(0,0)=2; dense(0,3)=-1; dense(2,3)=5;
    throws<std::out_of_range>([&] { (void)dense(3,0); });
    throws<std::out_of_range>([&] { (void)dense(0,4); });
    const Vector expected=dense*Vector{3,0,0,2};
    for(std::size_t i=0; i<y.size(); ++i) require(y[i]==expected[i], "Index convention mismatch");
}

}
int main(int argc, char** argv) {
    try {
        const std::string name=argc==2 ? argv[1] : "";
        if (name=="construction") construction();
        else if (name=="conversion") conversion();
        else if (name=="invalid_structure") invalid_structure();
        else if (name=="matvec") matvec();
        else if (name=="dense_reference") dense_reference();
        else if (name=="diagonal") diagonal_cases();
        else if (name=="norm_inf") norm_inf_cases();
        else if (name=="symmetry") symmetry_cases();
        else if (name=="basic_ops_reference") basic_ops_reference();
        else if (name=="storage_scaling") storage_scaling();
        else if (name=="zero_based_indices") zero_based_indices();
        else throw std::runtime_error("Unknown test case");
        std::cout << name << ": PASS\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
