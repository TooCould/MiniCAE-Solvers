#pragma once

#include <cstddef>          // std::size_t
#include <initializer_list> // std::initializer_list
#include <vector>           // std::vector

namespace minicae {

    class Vector {
    public:
        // 1. 默认构造函数：创建空向量
        Vector() = default;

        // 2. 指定长度构造函数：元素默认初始化为 0.0
        explicit Vector(const std::size_t n, const double initial_value = 0.0);

        // 3. 初始化列表构造函数
        Vector(std::initializer_list<double> values);

        // 4. 查询向量长度
        std::size_t size() const noexcept;

        // 5. 元素访问：可读写
        double &operator[](std::size_t i);

        // 6. 元素访问：只读
        const double &operator[](std::size_t i) const;

    private:
        std::vector<double> data_;
    };

    // 基础运算
    // 向量加减
    Vector operator+(const Vector &x, const Vector &y);
    Vector operator-(const Vector &x, const Vector &y);

    // 标量乘法
    Vector operator*(double alpha, const Vector &x);
    Vector operator*(const Vector &x, double alpha);

    // 向量内积
    double dot(const Vector &x, const Vector &y);

    // L2 范数
    double norm2(const Vector &x);

    // 取反
    Vector operator-(const Vector &x);
    // 原地取反：直接修改向量
    void negate(Vector &x);

    // 算法常用操作
    // y = alpha * x + y
    void axpy(double alpha, const Vector &x, Vector &y);

} // namespace minicae