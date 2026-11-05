# MiniCAE Framework

通用数值代数与 CAE 求解器框架实施路线

-   **版本**：V1.0 实施计划（三层核心架构修订）
-   **开发语言**：C++17 + Python
-   **核心目标**：围绕 CAE
    数值代数算法，完成从矩阵数据结构、迭代求解、预条件、稀疏直接法到
    PETSc 集成的一套可运行、可验证的小型框架。
-   **第一版范围**：Phase 0–6
-   **后续扩展**：多重网格与
    AMG、特征值算法深入、并行计算、专项性能分析等。

------------------------------------------------------------------------

## 1. 项目定位与边界

### 1.1 项目定位

MiniCAE Framework 是一个小型、模块化、可扩展的科学计算与 CAE
数值求解框架。

第一阶段以通用线性系统为核心：

$$
Ax=b
$$

其中，底层数值代数模块只处理矩阵、向量和算法，不依赖矩阵来自流体、结构、热传导还是其他物理问题。

在应用与基准层中，提供轻量级 Poisson 算例模块，使用二维 Poisson
方程作为矩阵装配、边界处理和数值验证的标准算例，不单独设立通用 PDE 核心层。

### 1.2 本项目要完成的能力

1.  实现向量、稠密矩阵及 COO、CSR 等基础数据结构。
2.  实现稀疏矩阵运算和 SpMV。
3.  实现 Jacobi、Gauss-Seidel、CG、GMRES(m) 等已学习的迭代方法。
4.  实现 Jacobi、ILU(0)、ICC(0) 预条件器。
5.  实现基础 LU、Cholesky 及稀疏消元过程的教学性验证。
6.  建立统一的求解器、预条件器接口。
7.  实现二维 Poisson 离散、稀疏装配和 Dirichlet 自由度消去。
8.  集成 PETSc 单进程求解流程，并使用 MUMPS 作为稀疏直接法对照。
9.  建立可重复的正确性测试和基础性能对比。

### 1.3 明确不做的事情

V1.0 不开发：

-   具体物理应用求解器；
-   毕业论文相关模型或数据生成流程；
-   GUI、复杂可视化界面；
-   完整网格前处理和后处理软件；
-   工业级多波前稀疏直接求解器；
-   自研工业级 AMG；
-   MPI、OpenMP 和分布式并行；
-   与数值代数学习目标无关的通用软件功能。

这些内容不属于当前交付范围。后续扩展只围绕数值代数知识结构展开。

------------------------------------------------------------------------

## 2. 总体架构

采用三层核心架构，以 Engineering & Validation 作为横向工程设施支撑所有层。
核心层按“数据与运算 → 数值算法 → 应用与基准”组织，工程设施不作为第四层。

```text
MiniCAE Framework
│
├── Layer 1: Linear Algebra Core
│   ├── Vector / DenseMatrix
│   ├── COO / CSR
│   └── Basic Matrix Operations / SpMV
│
├── Layer 2: Numerical Algorithms
│   ├── Iterative Solvers: Jacobi / GS / CG / GMRES(m)
│   ├── Krylov Building Blocks: Arnoldi
│   ├── Preconditioners: Jacobi / ILU(0) / ICC(0)
│   └── Direct Solvers: LU / Cholesky / Sparse Elimination
│
└── Layer 3: Applications & Benchmarks
    ├── Standard Matrices / Known-Solution Systems
    ├── Poisson: Grid / DOF / Stencil / Assembly / Boundary Conditions
    └── Solver / Preconditioner Comparison

Engineering & Validation（横向支撑 Layer 1–3）
├── CMake / CTest
├── Python / NumPy / SciPy Validation
├── Data Exchange / Timing / Experiment Records
└── Optional PETSc / MUMPS Adapters & Reference Runs
```

### 2.1 Layer 1：Linear Algebra Core

提供向量、稠密与稀疏矩阵、格式转换、SpMV、对角提取和基础矩阵性质检查。
该层不包含求解器、预条件器、网格或 PDE 专属逻辑，也不依赖上层模块。

### 2.2 Layer 2：Numerical Algorithms

依赖 Linear Algebra Core，实现迭代求解、Krylov 基构造、预条件与直接分解，
统一组织 Solver、Preconditioner 及分解因子复用接口。
该层只处理线性代数对象，不依赖物理问题、应用算例、Python 或 PETSc。

### 2.3 Layer 3：Applications & Benchmarks

组织标准矩阵、已知解线性系统、Poisson 算例和求解器比较实验。
Poisson 所需的规则网格、DOF 映射、五点模板、装配和边界处理均归入该层，
V1.0 不抽象为独立的通用 PDE 框架。

该层调用算法层求解，也可直接使用基础层生成矩阵、构造右端项和检查结果，
不重新实现基础矩阵结构或求解算法。基准问题、算法组合和比较条件由本层定义，
计时、记录与独立参考验证由横向设施支撑。

### 2.4 横向设施：Engineering & Validation

负责 CMake 构建、CTest 测试入口、Python 独立验证、数据交换、基础计时和实验记录，
贯穿三层核心架构。各层测试按被测模块组织，统一由工程设施执行。

PETSc / MUMPS 通过独立适配器和参考运行程序提供工程级对照，不成为自研核心库的
必需依赖。未配置 PETSc 时，仍应能够构建核心库、运行自主算法与非 PETSc 测试；
V1.0 最终验收仍须完成 Phase 6 的 PETSc / MUMPS 实际对照。

### 2.5 依赖与实施顺序

-   核心依赖方向：Applications & Benchmarks → Numerical Algorithms → Linear Algebra Core。
-   应用层可直接使用基础层，基础层和算法层不得反向依赖应用层。
-   PETSc 适配器可读取核心矩阵、向量并转换为 PETSc 对象，核心接口不暴露 PETSc 类型。
-   架构层次不等于开发阶段。Phase 2 先建立矩阵生成和装配能力，使用小规模参考解验证；
    Phase 3–5 再逐步接入自主求解器并补齐端到端实验，Phase 6 加入 PETSc 对照。

------------------------------------------------------------------------

## 3. 技术方案

### 3.1 语言分工

#### C++17：核心实现

负责：

-   Vector、DenseMatrix；
-   COO、CSR；
-   SpMV 和基础矩阵运算；
-   Jacobi、Gauss-Seidel、CG、GMRES；
-   Jacobi、ILU(0)、ICC(0)；
-   LU、Cholesky；
-   二维 Poisson 矩阵装配；
-   PETSc C/C++ 工程接口集成。

核心算法由自己实现，不能仅通过调用 SciPy 或 PETSc 来代替。

#### Python：独立验证与实验

负责：

-   使用 NumPy / SciPy 构造参考矩阵和参考解；
-   生成测试数据；
-   验证 C++ 输出；
-   计算误差、残差和收敛阶；
-   整理迭代次数、耗时和测试结果；
-   绘制必要的收敛与性能图表。

Python 作为独立参考环境，不重复开发生产版 C++ 核心算法。

### 3.2 工具

| 工具 | 用途 |
| --- | --- |
| C++17 | 核心数值算法 |
| CMake | 构建管理 |
| CTest | C++ 测试入口 |
| Python | 独立验证和实验 |
| NumPy | 向量、稠密矩阵参考计算 |
| SciPy | 稀疏矩阵和参考求解 |
| PETSc | 工程级求解器框架对照 |
| MUMPS | 稀疏直接法后端 |
| Hypre | 后续 AMG 阶段使用 |
| Git | 版本管理 |

V1.0 不强制引入大型 C++
第三方数值库。单元测试框架可先使用轻量级自编断言；如后续测试规模扩大，再选择测试库。

### 3.3 C++ 与 Python 的数据交换

初版不引入 pybind11 等绑定层，避免增加不必要的构建复杂度。

优先采用简单、可检查的文件格式：

-   稀疏矩阵：Matrix Market；
-   向量和结果：CSV 或简单文本格式；
-   实验记录：CSV。

Python 读取 C++ 输出后，使用 SciPy / NumPy
独立复算。待核心功能稳定后，再评估是否需要更紧密的语言绑定。

### 3.4 索引约定

-   数学公式、矩阵行列、网格节点编号：从 1 开始。
-   C++ 数组、COO/CSR 内部存储索引：从 0 开始。
-   文档中的数学编号与代码索引必须明确区分，不混用。

------------------------------------------------------------------------

## 4. 开发推进总览

V1.0 按以下顺序推进，不并行铺开多个未完成模块。Phase 0 建立横向设施，
Phase 1 对应基础层，Phase 2 准备应用层测试问题，Phase 3–5 对应算法层，
Phase 6 完成横向 PETSc 集成并扩展应用层对照实验。CTest 与 Python 验证贯穿各阶段。

| 阶段 | 开发内容 | 主要成果 | 课程对应 |
| --- | --- | --- | --- |
| Phase 0 | 项目基础设施 | 可编译、可测试的工程骨架 | C++ / CMake |
| Phase 1 | 矩阵与向量 | Dense、COO、CSR、SpMV | 1.1–1.3 |
| Phase 2 | 标准测试系统与 Poisson 装配 | 标准矩阵、Poisson、边界处理、数值验证 | 1.4–1.7、6.4 |
| Phase 3 | 迭代求解器 | Jacobi、GS、CG、GMRES(m) | 2.1–2.5 |
| Phase 4 | 预条件器 | Jacobi、ILU(0)、ICC(0) | 3.1–3.6 |
| Phase 5 | 直接法 | LU、Cholesky、稀疏消元基础 | 4.1–4.5 |
| Phase 6 | PETSc 集成 | KSP、PC、MUMPS 和算法对照 | 7.1–7.8 |

每个阶段均遵循：

1.  明确本阶段输入、输出和数学对象。
2.  先完成最小实现。
3.  编写小规模手算测试。
4.  编写自动化测试。
5.  使用 Python / SciPy 做独立对照。
6.  通过验收后再进入下一阶段。
7.  整理本阶段算法原理、复杂度和面试表达。

------------------------------------------------------------------------

## 5. Phase 0：项目基础设施

### 5.1 目标

建立一个可以持续扩展的 C++ / Python 项目，不在此阶段开发复杂功能。

### 5.2 任务

1.  创建 Git 仓库。
2.  建立 CMake 工程。
3.  固定 C++17 标准。
4.  建立头文件、源文件、测试和示例目录。
5.  配置 CTest。
6.  建立 Python 验证目录。
7.  建立统一的构建、测试命令。

### 5.3 建议目录结构

```text
MiniCAE/
├── CMakeLists.txt
├── README.md
├── include/
│   ├── linalg/                 # Layer 1：向量、矩阵、稀疏存储和基础运算
│   └── algorithms/             # Layer 2：数值算法公共接口
│       ├── solvers/
│       ├── preconditioners/
│       └── direct/
├── src/
│   ├── linalg/
│   └── algorithms/
│       ├── solvers/
│       ├── preconditioners/
│       └── direct/
├── applications/               # Layer 3：算例与基准问题公共代码
│   ├── matrices/
│   └── poisson/                # Grid / DOF / Stencil / Assembly / BC
├── examples/                   # Layer 3：线性系统与 Poisson 运行入口
├── benchmarks/                 # Layer 3：求解器组合与对比实验入口
├── tests/                      # 横向验证，按被测模块组织
│   ├── linalg/
│   ├── algorithms/
│   ├── applications/
│   └── integrations/
├── integrations/                # 横向设施：可选后端适配与参考运行
│   └── petsc/
├── engineering/                 # 横向设施：数据交换、计时和记录工具
├── python/                     # 横向设施：独立验证与图表
│   ├── reference/
│   ├── validation/
│   └── plots/
└── docs/
```

目录随阶段按需建立，不在 Phase 0 提前填充算法或应用功能。CMake 目标按三层拆分，
通过独立选项启用 PETSc 集成，保持核心目标不依赖 PETSc。

### 5.4 验收标准

-   CMake 可以完成配置和编译。
-   最小 C++ 程序能够运行。
-   CTest 能执行至少一个测试。
-   Python 能运行 NumPy / SciPy 验证脚本。
-   Git 仓库结构清晰，生成文件和构建目录不进入版本控制。

**完成标志：** 工程骨架稳定，后续只需增加模块，不需要反复调整整体工程。

------------------------------------------------------------------------

## 6. Phase 1：矩阵与向量基础设施

**课程对应：1.1、1.2、1.3。**

### 6.1 目标

实现后续所有数值算法共同依赖的基础数据结构和矩阵运算。

### 6.2 Vector

实现：

-   指定长度创建；
-   初始化和元素访问；
-   向量加减；
-   数乘；
-   内积；
-   L2 范数；
-   AXPY：$y \leftarrow \alpha x+y$。

测试：

-   零向量；
-   单位向量；
-   手算内积；
-   手算 L2 范数；
-   AXPY 与 NumPy 对照。

### 6.3 DenseMatrix

实现：

-   指定行列数创建；
-   元素访问；
-   矩阵转置；
-   矩阵向量乘法；
-   小规模矩阵乘法。

测试：

-   非方阵；
-   零矩阵；
-   单位矩阵；
-   手算矩阵乘法；
-   与 NumPy 对照。

### 6.4 COO

实现：

-   row；
-   col；
-   values；
-   非零元素插入；
-   重复坐标累加；
-   维度和索引合法性检查；
-   nnz 统计。

测试：

-   单个非零元；
-   多个非零元；
-   重复坐标 `(row, col)`；
-   空行；
-   非方阵；
-   越界索引。

### 6.5 CSR

实现：

-   values；
-   col_indices；
-   row_ptr；
-   COO → CSR；
-   CSR 结构合法性检查；
-   CSR SpMV。

必须检查：

$$
\operatorname{len}(row_ptr)=n+1
$$

$$
\operatorname{len}(values) =
\operatorname{len}(col_indices) = nnz
$$

$$
row_ptr[n]=nnz
$$

以及：

$$
row_ptr[i+1]\geq row_ptr[i]
$$

### 6.6 基础矩阵操作

实现：

-   矩阵维度查询；
-   nnz 查询；
-   对角元素提取；
-   矩阵无穷范数或其他明确实现的基础范数；
-   对称性检查；
-   CSR 转置（若实现成本可控，可放在本阶段末尾）。

CSC 暂不作为 V1.0 核心数据结构；在直接法需要时再决定是否补充。

### 6.7 阶段验收

1.  COO → CSR 转换正确。
2.  重复 COO entries 能正确累加。
3.  CSR 结构检查能够发现非法 row_ptr 和列索引。
4.  SpMV 与 DenseMatrix、NumPy 结果一致。
5.  不同矩阵规模下，CSR 存储量符合 (O(nnz+n)) 的预期。
6.  代码中明确区分数学编号和 0-based 数组索引。

**完成标志：** 能够独立创建矩阵 (A) 和向量 (x)，计算
(y=Ax)，并通过独立参考结果验证。

------------------------------------------------------------------------

## 7. Phase 2：标准测试系统与 PDE 矩阵装配

**课程对应：1.4–1.7、6.4。**

### 7.1 目标

建立统一的矩阵测试和数值验证体系，并用二维 Poisson
方程验证从网格到稀疏矩阵的完整映射。

### 7.2 标准矩阵生成

生成以下矩阵：

| 类型 | 用途 |
| --- | --- |
| 对角矩阵 | 基础计算与 Jacobi |
| 三对角矩阵 | 一维 SPD 系统 |
| 五对角矩阵 | 二维 Poisson |
| 一般非对称矩阵 | GMRES |
| 病态矩阵 | 条件数与预条件实验 |
| 块稀疏矩阵 | 为后续块方法预留测试数据 |

测试矩阵应记录维度、nnz、对称性、构造方式和已知性质。不要仅凭矩阵外观断言其正定性。

### 7.3 已知离散解测试

人为指定：

$$
x^*
$$

计算：

$$
b=Ax^*
$$

再调用求解器求解：

$$
Ax=b
$$

比较：

-   相对解误差；
-   绝对残差；
-   相对残差；
-   后向误差；
-   求解器返回的收敛状态。

这组测试用于隔离验证线性求解器，不涉及连续 PDE 的离散误差。

### 7.4 二维 Poisson 离散

采用：

$$
-\Delta u=f
$$

实现：

1.  二维均匀网格；
2.  网格节点到一维 DOF 的映射；
3.  内部与边界自由度划分；
4.  五点差分 stencil；
5.  COO entries 生成；
6.  COO 重复项累加；
7.  COO → CSR；
8.  Dirichlet 边界条件处理；
9.  自由度消去和 RHS 修正。

对于内部自由度 (I) 和已知边界自由度 (B)：

$$
A_{II}x_I=b_I-A_{IB}g
$$

其中 (g) 是已知边界值向量。

### 7.5 PDE 离散验证

使用连续解析解，例如：

$$
u^*(x,y)=\sin(\pi x)\sin(\pi y)
$$

根据解析解计算对应的 (f) 和边界条件，再比较数值解与连续精确解。

验证：

-   网格加密；
-   离散误差；
-   收敛阶；
-   边界处理正确性；
-   求解器容差是否足以观察离散误差。

注意：离散已知解测试与连续解析解测试是两种不同验证，不能混为一谈。

### 7.6 阶段验收

-   标准矩阵生成正确。
-   Poisson 五点模板装配正确。
-   COO → CSR 后矩阵与参考矩阵一致。
-   Dirichlet 自由度消去和 RHS 修正正确。
-   离散已知解测试通过。
-   连续解析解测试能够观察合理的网格收敛趋势。
-   独立计算真实残差，不只依赖求解器内部收敛标志。

**完成标志：** 形成可重复的标准线性系统与 Poisson
测试环境，后续所有求解算法共用这一环境。

------------------------------------------------------------------------

## 8. Phase 3：迭代求解器

**课程对应：2.1–2.5。**

### 8.1 目标

将已经学习的迭代算法实现为统一接口，重点掌握 CG 和 GMRES。

### 8.2 Stationary Iteration

实现：

-   Jacobi；
-   Gauss-Seidel。

测试不同初值、对角占优程度和迭代停止条件，观察收敛与发散。

### 8.3 Conjugate Gradient

适用条件：实对称正定矩阵。

实现：

1.  初始解和初始残差；
2.  初始搜索方向；
3.  SpMV：(Ap_k)；
4.  $\alpha_k$ 计算；
5.  解和残差更新；
6.  $\beta_k$ 计算；
7.  搜索方向更新；
8.  收敛判断。

验证：

-   SPD 系统；
-   不同条件数；
-   不同初始解；
-   与 SciPy CG 对照；
-   独立真实残差。

### 8.4 Arnoldi

实现：

-   初始 Krylov 基；
-   Modified Gram-Schmidt；
-   正交投影系数；
-   Hessenberg 矩阵；
-   新基向量归一化；
-   正交性检查。

验证：

$$
AV_k=V_{k+1}\bar H_k
$$

检查 Arnoldi 关系误差和基向量正交误差。

### 8.5 GMRES

实现：

1.  初始残差；
2.  Arnoldi 基构造；
3.  Hessenberg 矩阵更新；
4.  Givens Rotation；
5.  小型最小二乘问题；
6.  解向量恢复；
7.  收敛判断。

### 8.6 Restarted GMRES

实现 GMRES(m)：

-   restart 长度；
-   当前近似解保留；
-   重新计算残差；
-   重新构造 Krylov 子空间；
-   最大迭代次数管理。

测试不同 restart 长度，观察内存、正交化成本和收敛速度。

### 8.7 统一接口

建议接口：

```cpp
solve(A, b, x, options);
```

返回结构至少包含：

-   converged；
-   convergence_reason；
-   iterations；
-   initial_residual；
-   final_reported_residual；
-   residual_history（按需记录）。

容差定义：

$$
\|b-Ax_k\|_2 \leq \max(atol,rtol\|b\|_2)
$$

算法内部可以使用递推或预条件残差，但最终验收必须独立计算真实残差。

### 8.8 阶段验收

-   Jacobi、Gauss-Seidel 能正确执行。
-   CG 在 SPD 系统上通过验证。
-   GMRES 在一般非对称系统上通过验证。
-   Arnoldi 关系和正交性满足预定数值容差。
-   GMRES(m) 能正确 restart。
-   不适用矩阵类型能够给出明确诊断。
-   所有算法统一返回迭代状态和残差信息。

**完成标志：**
能够根据矩阵性质选择合适的迭代方法，并解释其算法机制和适用条件。

------------------------------------------------------------------------

## 9. Phase 4：预条件方法

**课程对应：3.1–3.6。**

### 9.1 目标

实现预条件器，并使 Solver 与 Preconditioner 相互独立。

### 9.2 统一预条件接口

预条件器提供：

-   setup(A)；
-   apply(v, z)。

其中 apply 表示求解：

$$
Mz=v
$$

并返回：

$$
z=M^{-1}v
$$

禁止显式构造 (M^{-1})。

### 9.3 Jacobi Preconditioner

构造：

$$
M=\operatorname{diag}(A)
$$

实现对角提取、零对角检查和逐元素缩放。

### 9.4 ILU(0)

实现：

-   稀疏行分解；
-   原始稀疏模式保留；
-   不允许超出原模式的 fill-in；
-   三角前代；
-   三角回代；
-   零主元或数值 breakdown 处理。

验证：

$$
A\approx LU
$$

并统计分解误差和存储量。

### 9.5 ICC(0)

实现：

-   对称稀疏结构处理；
-   不完全 Cholesky 分解；
-   三角求解；
-   分解失败检测。

验证：

$$
A\approx LL^T
$$

ICC 只用于满足相应 SPD
条件的系统，不能把它当作一般非对称矩阵的预条件器。

### 9.6 左、右预条件

先完成左预条件，再实现右预条件。

左预条件：

$$
M^{-1}Ax=M^{-1}b
$$

右预条件：

$$
AM^{-1}y=b,\qquad x=M^{-1}y
$$

两种实现都必须保留原始系统真实残差的独立验收。

### 9.7 组合测试

至少完成：

-   CG；
-   CG + Jacobi；
-   CG + ICC(0)；
-   GMRES；
-   GMRES + ILU(0)。

比较：

-   预条件建立时间；
-   迭代次数；
-   单次迭代成本；
-   总耗时；
-   真实残差；
-   预条件存储量。

### 9.8 阶段验收

-   预条件器不显式构造矩阵逆。
-   Solver 不依赖某一种具体预条件器。
-   ILU(0) 保留原始稀疏模式。
-   ICC(0) 能识别不满足分解条件的输入。
-   CG 与 SPD 兼容预条件器组合正确。
-   GMRES + ILU(0) 能正确求解一般非对称系统。
-   实验能区分 setup cost、iteration count 和总耗时。

**完成标志：** 能独立实现和组合 Solver /
Preconditioner，并通过实验评价预条件效果。

------------------------------------------------------------------------

## 10. Phase 5：直接法与稀疏消元

**课程对应：4.1–4.5。**

### 10.1 目标

实现基础直接求解流程，理解稀疏分解、fill-in、ordering 和多右端项复用。

### 10.2 Dense Gaussian Elimination

实现：

-   高斯消元；
-   部分主元选取；
-   消元；
-   回代；
-   奇异或近似奇异主元检测。

### 10.3 LU

实现：

$$
PA=LU
$$

其中 (P) 表示主元置换。

实现：

-   分解；
-   前代；
-   回代；
-   多 RHS；
-   因子复用。

### 10.4 Cholesky

针对 SPD 矩阵实现：

$$
A=LL^T
$$

实现：

-   分解；
-   前代；
-   回代；
-   输入条件检查。

### 10.5 Sparse Factorization 教学实现

以小规模稀疏矩阵为对象，验证：

-   稀疏消元；
-   fill-in 产生；
-   symbolic factorization 与 numerical factorization 的区别；
-   因子非零元数量；
-   不同变量排列对 fill-in 的影响。

本阶段不开发工业级多波前稀疏直接求解器。

AMD、Nested Dissection
的图算法细节暂不自行复刻；先掌握其作用，并通过小矩阵或成熟后端观察排序前后的填充差异。

### 10.6 Multiple RHS

对同一个 (A)，求解多个右端项：

$$
AX=B
$$

比较：

-   重复分解；
-   分解一次、重复前代和回代。

### 10.7 阶段验收

-   LU 与 Cholesky 小规模测试正确。
-   部分主元处理有效。
-   奇异矩阵能够给出明确错误。
-   多 RHS 能复用因子。
-   稀疏消元能够展示 fill-in。
-   能解释 symbolic / numerical factorization、ordering 和 factors
    的关系。

**完成标志：** 掌握基础直接法实现，并能解释稀疏直接法的主要成本来源。

------------------------------------------------------------------------

## 11. Phase 6：PETSc 工程集成

**课程对应：7.1–7.8。**

### 11.1 目标

使用 PETSc 建立实际工程求解器接口，与自主实现算法对照。

第一版只使用单进程 sequential Vec / Mat，不进入 MPI 分布式计算。

### 11.2 PETSc 基础对象

掌握：

-   Vec；
-   Mat；
-   KSP；
-   PC。

理解矩阵装配、对象生命周期、索引类型、数值类型及 C++ RAII 资源管理。

### 11.3 Mat / Vec

实现：

-   sequential Vec；
-   sequential AIJ 稀疏矩阵；
-   稀疏矩阵装配；
-   COO / CSR 数据导入；
-   SpMV 一致性验证。

使用 SciPy CSR 和 PETSc AIJ 对相同矩阵、相同随机向量进行对照。

### 11.4 KSP

配置并验证：

-   KSPCG；
-   KSPGMRES；
-   restart；
-   tolerance；
-   max_it；
-   convergence reason；
-   monitor。

### 11.5 PC

配置并验证：

-   PCJACOBI；
-   PCILU；
-   PCICC；
-   PCLU。

具体 PC 类型是否可用，以当前 PETSc
构建配置为准。不可用时明确报告，不允许静默换成其他算法。

### 11.6 MUMPS

通过 PETSc 调用 MUMPS：

-   检查 PETSc 是否实际包含 MUMPS 支持；
-   配置稀疏 LU 后端；
-   验证求解结果；
-   区分分析、分解和求解阶段；
-   验证多 RHS 或因子复用能力。

### 11.7 Solver 对照实验

在同一矩阵、右端项、初始解、容差和硬件条件下比较：

1.  自主实现 CG；
2.  自主实现 GMRES；
3.  自主实现 CG + Jacobi；
4.  自主实现 GMRES + ILU(0)；
5.  PETSc CG + Jacobi；
6.  PETSc GMRES + ILU；
7.  PETSc + MUMPS LU。

所有结果必须记录收敛状态和独立真实残差。未收敛结果不能从实验记录中删除。

### 11.8 性能记录

基础记录：

-   matrix size；
-   nnz；
-   solver；
-   preconditioner；
-   iterations；
-   setup time；
-   solve time；
-   total time；
-   true residual；
-   convergence reason。

计时实验应区分首次建立成本和重复求解成本。

### 11.9 阶段验收

-   PETSc 单进程 Vec / Mat 能正常创建和装配。
-   KSPCG、KSPGMRES 能正常运行。
-   PC 配置与实际生效类型一致。
-   MUMPS 后端经过实际求解验证，而不只是检查依赖是否存在。
-   PETSc 与自主实现使用同一测试矩阵和验收标准。
-   所有算法均通过独立真实残差检查。

**完成标志：** 形成自主算法与 PETSc 工程求解器的可复现实验对照。

------------------------------------------------------------------------

## 12. V1.0 最终交付标准

V1.0 只在 Phase 0–6 完成后验收。

### 12.1 必须交付

#### Layer 1：Linear Algebra Core

-   Vector；
-   DenseMatrix；
-   COO；
-   CSR；
-   SpMV 与基础矩阵操作。

#### Layer 2：Numerical Algorithms

-   Jacobi；
-   Gauss-Seidel；
-   CG；
-   Arnoldi；
-   GMRES(m)；
-   Jacobi Preconditioner；
-   ILU(0)；
-   ICC(0)；
-   Dense LU；
-   Cholesky；
-   基础稀疏消元教学实现。

#### Layer 3：Applications & Benchmarks

-   标准矩阵与已知解线性系统；
-   求解器与预条件器比较实验；
-   二维均匀网格；
-   DOF Mapping；
-   五点差分；
-   COO 装配；
-   CSR 转换；
-   Dirichlet 消元；
-   RHS 修正；
-   离散已知解测试；
-   连续解析解测试；
-   网格收敛测试。

#### 横向设施：Engineering & Validation

-   CMake / CTest；
-   数据交换、计时与实验记录工具；
-   Python 独立验证；
-   PETSc 单进程集成；
-   MUMPS 实际求解对照；
-   基础性能记录。

### 12.2 最终验收原则

1.  代码能够从干净构建目录重新编译。
2.  核心单元测试全部通过。
3.  Python 参考验证通过。
4.  求解器收敛状态与真实残差一致。
5.  不适用的矩阵类型或失败分解能够明确报告。
6.  算法比较使用相同测试条件。
7.  文档区分自主实现、第三方库调用和尚未实现的功能。
8.  不夸大规模、性能提升或工业应用能力。

------------------------------------------------------------------------

## 13. 后续阶段路线（不属于 V1.0）

以下内容在 V1.0 完成后，根据学习需要逐步补充。

### 13.1 Phase 7：多重网格与 AMG

课程对应：第 5 章。

顺序：

1.  高频误差、低频误差和 smoothing property；
2.  Geometric Multigrid；
3.  Restriction / Prolongation；
4.  Coarse-grid Correction；
5.  V-cycle；
6.  AMG；
7.  Strong Connection、Coarsening、Interpolation；
8.  Galerkin Coarse Operator；
9.  Hypre BoomerAMG；
10. PETSc PCHYPRE；
11. CG + AMG、GMRES + AMG。

先理解并实现基础两层或 V-cycle，再集成成熟 AMG 后端。不以复刻工业级 AMG
为目标。

### 13.2 Phase 8：特征值与高级矩阵操作

课程对应：第 6 章。

顺序：

1.  Power Iteration；
2.  Lanczos；
3.  Arnoldi 特征值方法；
4.  Shift-and-Invert；
5.  SLEPc / EPS；
6.  Static Condensation；
7.  Schur Complement；
8.  Block System。

### 13.3 Phase 9：数值性能与求解器诊断

课程对应：第 9、11 章。

顺序：

1.  Assembly / Setup / Solve / Verification 计时；
2.  内存占用；
3.  SpMV 与向量归约成本；
4.  收敛停滞和发散诊断；
5.  Singular Matrix、Ill-conditioning、Bad Scaling；
6.  Zero Row、Zero Diagonal、NaN / Inf；
7.  Solver / Preconditioner 选择错误；
8.  PETSc Profiling；
9.  Linux `time` / `perf`；
10. 性能异常定位与调优。

### 13.4 Phase 10：MPI 与 OpenMP

课程对应：第 8、10 章。

顺序：

1.  MPI Process / Rank / Communicator；
2.  矩阵与向量分区；
3.  Distributed CSR；
4.  Ghost / Halo；
5.  Parallel SpMV；
6.  Parallel Krylov；
7.  Block Jacobi / Domain Decomposition；
8.  OpenMP；
9.  数据竞争和 Reduction；
10. MPI + OpenMP。

并行开发放在单进程算法和 PETSc 流程稳定之后。

### 13.5 Phase 11：面试与项目整合

课程对应：第 12 章。

整理：

-   算法原理；
-   矩阵性质与 Solver 选择；
-   Krylov 与预条件；
-   稀疏直接法；
-   AMG；
-   PETSc；
-   数值正确性验证；
-   性能分析；
-   求解失败诊断；
-   项目架构和代码说明。

所有简历和面试表述以实际实现、测试和测量结果为依据。

------------------------------------------------------------------------

## 14. 每轮推进与验收记录

开发按功能块推进，不一次性铺开全部代码。

每个功能块统一记录：

```text
模块名称：
对应课程章节：
前置知识：
输入与输出：
核心算法：
实现文件：
单元测试：
Python 参考验证：
验收结果：
遗留问题：
```

每轮结束后：

1.  检查代码结构和接口；
2.  运行全部相关测试；
3.  对照 Python / SciPy 参考结果；
4.  解释核心算法和关键公式；
5.  完成简短知识检查；
6.  通过后进入下一功能块。

如果验收失败，先修正当前模块，不带着未解决的问题进入下一阶段。

------------------------------------------------------------------------

## 15. 项目执行顺序与当前起点

```text
Phase 0  项目基础设施
   ↓
Phase 1  Vector / Dense / COO / CSR / SpMV
   ↓
Phase 2  标准矩阵 / Poisson / 边界 / 数值验证
   ↓
Phase 3  Jacobi / GS / CG / Arnoldi / GMRES(m)
   ↓
Phase 4  Jacobi PC / ILU(0) / ICC(0)
   ↓
Phase 5  LU / Cholesky / Sparse Elimination
   ↓
Phase 6  PETSc / KSP / PC / MUMPS
   ↓
       V1.0 完成
   ↓
Phase 7  Multigrid / AMG
   ↓
Phase 8  Eigenvalue / Advanced Matrix Operations
   ↓
Phase 9  Performance / Solver Diagnostics
   ↓
Phase 10 MPI / OpenMP
   ↓
Phase 11 Interview / Project Integration
```

**当前起点：Phase 0。**

第一项工作是建立 C++17 + CMake + CTest + Python
的最小工程骨架。完成并验收后，立即进入 Phase 1 的
Vector，不提前开发后续算法。

------------------------------------------------------------------------

## 附录 A：课程知识与项目模块对照

| 数值代数课程 | 项目阶段 | 实现或验证内容 |
| --- | --- | --- |
| 1.1–1.3 | Phase 1 | 矩阵、向量、COO、CSR、SpMV |
| 1.4–1.5 | Phase 2 | 矩阵性质、条件数概念、误差、残差、收敛 |
| 1.6 | Phase 2 | 稀疏装配、Dirichlet、RHS 修正 |
| 1.7 | Phase 2 | 精确解、PDE 验证、网格收敛 |
| 2.1 | Phase 3 | 直接法与迭代法框架、Stationary Iteration |
| 2.2 | Phase 3 | Krylov 子空间 |
| 2.3 | Phase 3 | CG |
| 2.4–2.5 | Phase 3 | Arnoldi、GMRES、Restart |
| 2.6 | 后续补充 | MINRES、BiCGStab 等 |
| 3.1–3.3 | Phase 4 | 预条件基础、左右预条件、Jacobi |
| 3.4 | 后续补充 | Gauss-Seidel / SOR 预条件 |
| 3.5–3.6 | Phase 4 | ILU、ICC、预条件器选择 |
| 4.1 | Phase 5 | Gaussian Elimination、LU、Cholesky、QR 概念 |
| 4.2–4.4 | Phase 5 | 稀疏分解、Fill-in、Ordering |
| 4.5 | Phase 6 | MUMPS、PETSc |
| 5.1–5.6 | Phase 7 | Multigrid、AMG、Hypre |
| 6.1 | Phase 2 / 后续 | PDE 测试中的矩阵性质；特征值问题基础 |
| 6.2–6.3 | Phase 8 | Lanczos、Arnoldi、SLEPc |
| 6.4 | Phase 2 | Dirichlet 自由度消去 |
| 6.5–6.6 | Phase 8 | Static Condensation、Schur Complement |
| 7.1–7.8 | Phase 6 | PETSc 单进程求解与对照 |
| 8 | Phase 10 | MPI 并行数值代数 |
| 9 | Phase 9 | 性能分析 |
| 10 | Phase 10 | OpenMP |
| 11 | Phase 9 | 求解器诊断与调优 |
| 12 | Phase 11 | 面试与项目整合 |

## 附录 B：开发约束

-   V1.0 只要求单进程、单机运行。
-   优先保证算法正确性，不提前优化底层性能。
-   统一使用 C++17。
-   Python 用于独立验证，不替代 C++ 核心实现。
-   数学公式和矩阵编号从 1 开始；代码数组及 COO / CSR 索引从 0 开始。
-   每个模块必须有测试和明确验收标准。
-   不把尚未实现或未经验证的功能写成项目成果。
