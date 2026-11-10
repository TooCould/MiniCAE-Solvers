# MiniCAE-Solvers

使用 C++17 与 CMake 构建的小型数值代数学习项目。

## 当前进度

Phase 0：Windows 构建、运行、断点调试、CTest 和 Python 参考环境验证已通过；macOS Debug / Release 构建、运行、断点调试、CTest 和 Python 参考环境也已验证。

Phase 1：Vector、DenseMatrix、COO、CSR、SpMV 及基础矩阵操作已实现。macOS Debug / Release 各 67 项 CTest 通过；独立对照分别为 Vector 11 组、DenseMatrix 12 + 14 组、COO 14 组、CSR 26 组，均通过。Windows Release 既有模块独立对照及新增 CSR 验证待完成。详见 `docs/MiniCAE-Solvers-Framework.md`。

## Visual Studio 文件夹开发

1. 使用“文件 → 打开 → 文件夹”打开项目根目录。
2. 等待 CMake 读取 `CMakePresets.json`，选择 `windows-debug`（显示名称为 `Windows x64 Debug`）。
3. 使用“生成 → 全部生成”编译，不需要启动 Build Insights 性能采集。
4. 在顶部“选择启动项”中选择 `minicae_demo.exe`。
5. 按 `Ctrl+F5` 运行；在 `src/main.cpp` 输出语句处设置断点后，按 `F5` 调试。
6. 打开“测试 → 测试资源管理器”，运行 `minicae_startup`。

程序正常运行时输出当前平台与构建模式，例如：

```text
MiniCAE-Solvers (Windows Debug): C++17 startup OK
```

切换并重新编译后，会自动显示 `Windows Release`、`macOS Debug` 或 `macOS Release`。
信息由 CMake 在编译时传入，反映当前运行的可执行文件所用的构建配置。

## 命令行构建与测试

需安装 CMake 3.21 或更新版本，以及支持 C++17 的编译器。配置、编译、测试命令均在项目根目录执行。

### Windows

在 Visual Studio 的 x64 Native Tools 命令提示符（CMD）中执行。命令行还需可找到 CMake、CTest 和 Ninja；VS 的 C++ CMake 工具包含这些程序，VS 界面会负责设置构建环境。

```bat
cmake --preset windows-debug
cmake --build --preset windows-debug
ctest --preset windows-debug
```

需要 Release 时，将三条命令中的 `windows-debug` 均替换为 `windows-release`。

### macOS

安装 Xcode Command Line Tools 和 CMake 后执行。预设使用系统 Apple Clang 和 Make，默认构建本机架构，适用于 Apple Silicon 或 Intel Mac。

```sh
cmake --preset macos-debug
cmake --build --preset macos-debug
ctest --preset macos-debug
```

需要 Release 时，将三条命令中的 `macos-debug` 均替换为 `macos-release`。

### 配置文件与构建目录

`CMakeLists.txt` 定义程序和测试；`CMakePresets.json` 保存共享的构建配置，应纳入版本管理。
机器专属配置可放入已被忽略的 `CMakeUserPresets.json`，不要将本机绝对工具路径写入共享预设。

如果 VS 仍只显示旧的 `x64-Debug` 配置，先关闭并重新打开项目文件夹；若仍未切换，在“工具 → 选项”中搜索 `CMake`，启用使用 `CMakePresets.json` 的选项，再重新打开文件夹。

## Python 独立验证环境

使用 Python 3.13.15。两台机器共用 `python/requirements.txt`，分别建立自己的 `.venv`，不复制或提交虚拟环境目录。以下命令均在项目根目录执行，直接调用环境中的 Python，无需激活环境。

### Windows CMD（命令提示符）

```bat
python -m venv .venv
.venv\Scripts\python.exe -m pip install --upgrade pip
.venv\Scripts\python.exe -m pip install -r python\requirements.txt
.venv\Scripts\python.exe python\validation\check_environment.py
```

### macOS 终端

先运行 `python --version`，确认该命令指向 Python 3.13.15，再执行：

```sh
python -m venv .venv
./.venv/bin/python -m pip install --upgrade pip
./.venv/bin/python -m pip install -r python/requirements.txt
./.venv/bin/python python/validation/check_environment.py
```

若已创建 `.venv`，跳过创建命令。脚本检查稠密与 CSR 矩阵向量乘法、已知解线性系统的求解结果和真实残差。成功时输出 `Python reference environment: PASS`；检查失败时返回非零退出码。
当前脚本只验证 Python 参考工具链，后续各 C++ 数值模块还需单独对照验证。

## Vector 与 NumPy 独立对照

`export_vector` 是独立的验证辅助程序，调用项目中实际实现的 Vector，读取标准输入并将计算结果以 CSV 输出。Python 脚本生成相同输入，运行该程序，再与 NumPy 的参考运算比较，不需要更换 VS 启动项。

覆盖 11 组输入：空向量、手算示例、单位向量、零系数、极大与极小数值，以及固定随机种子的不同长度向量。对照加减、左右数乘、取反、AXPY、内积和 L2 范数。范数参考采用 NumPy 的 `hypot.reduce`，避免直接平方导致中间溢出或下溢。

### Windows CMD

在 VS 的 x64 Native Tools 命令提示符中、项目根目录下执行：

```bat
cmake --build --preset windows-debug --target export_vector
.venv\Scripts\python.exe python\validation\check_vector.py --exe out\build\windows-debug\export_vector.exe --output-dir out\validation\vector\windows-debug
```

验证 Release 时，将上述命令中所有 `windows-debug` 替换为 `windows-release`。

### macOS 终端

```sh
cmake --build --preset macos-debug --target export_vector
./.venv/bin/python python/validation/check_vector.py --exe out/build/macos-debug/export_vector --output-dir out/validation/vector/macos-debug
```

macOS 首次构建前先执行 `cmake --preset macos-debug`。验证 Release 时，将上述命令中所有 `macos-debug` 替换为 `macos-release`。

成功时显示 `Vector NumPy validation: PASS (11 cases)`。输入、C++ 结果、NumPy 结果与成功汇总 `summary.csv` 保存在指定的输出目录；失败时脚本返回非零退出码，且不保留上一次的成功汇总。

当前对照程序的 Windows Debug / Release 构建与手算样例输出已验证；Windows Debug 的 11 组 NumPy 独立对照已通过，结果保存于 `out/validation/vector/windows-debug/`。macOS Debug / Release 的 11 组对照均已通过；Windows Release 对照待完成。NaN、无穷值及异常行为仍由 C++ CTest 覆盖，本脚本对照有限输入下的数值结果。

## DenseMatrix 转置与矩阵向量乘法的 NumPy 对照

`export_dense_matrix` 调用项目的 `transpose(A)` 与 `A * x`，输出实际计算结果；Python 独立用 `A.T` 和 `A @ x` 对照。12 组输入覆盖空矩阵、零行、零列、宽矩阵、长矩阵、单位矩阵、零矩阵、单行、单列和固定随机种子的不同形状矩阵。C++ 测试另外检查输入保持不变及尺寸不匹配异常。

### Windows CMD

```bat
cmake --build --preset windows-debug --target export_dense_matrix
.venv\Scripts\python.exe python\validation\check_dense_matrix.py --exe out\build\windows-debug\export_dense_matrix.exe --output-dir out\validation\dense_matrix\windows-debug
```

也可以在 VS 中“生成 → 全部生成”，然后只执行 Python 命令。Release 对照将所有 `windows-debug` 替换为 `windows-release`。

### macOS 终端

```sh
cmake --build --preset macos-debug --target export_dense_matrix
./.venv/bin/python python/validation/check_dense_matrix.py --exe out/build/macos-debug/export_dense_matrix --output-dir out/validation/dense_matrix/macos-debug
```

首次配置先执行 `cmake --preset macos-debug`。成功时显示 `DenseMatrix NumPy validation: PASS (12 cases)`，输入、C++ 结果、NumPy 结果及成功汇总保存在指定输出目录。Windows Debug 的 12 组 NumPy 对照已通过，结果位于 `out/validation/dense_matrix/windows-debug/`。macOS Debug / Release 的 12 组对照均已通过；Windows Release 对照待完成。

### 矩阵乘矩阵

同一脚本使用 `--matmul` 验证 `A * B`，NumPy 参考为 `A @ B`。14 组输入覆盖手算非方阵、左右单位矩阵、零矩阵、行乘列、列乘行、零行、零列、零内维及固定种子的随机矩阵。此前的转置与矩阵向量乘法模式保持不变。

Windows CMD：

```bat
cmake --build --preset windows-debug --target export_dense_matrix
.venv\Scripts\python.exe python\validation\check_dense_matrix.py --matmul --exe out\build\windows-debug\export_dense_matrix.exe --output-dir out\validation\dense_matrix\windows-debug
```

macOS 终端：

```sh
cmake --build --preset macos-debug --target export_dense_matrix
./.venv/bin/python python/validation/check_dense_matrix.py --matmul --exe out/build/macos-debug/export_dense_matrix --output-dir out/validation/dense_matrix/macos-debug
```

成功时显示 `DenseMatrix matmul NumPy validation: PASS (14 cases)`。矩阵乘矩阵的记录使用 `matmul_` 文件名前缀，成功汇总为 `summary_matmul.csv`，与此前的 12 组结果分别保存。Windows Debug / Release 的 C++ 测试与手算导出已通过；Windows Debug 的 14 组 NumPy 对照已验收，结果保存于 `out/validation/dense_matrix/windows-debug/summary_matmul.csv`。macOS Debug / Release 的 14 组对照均已通过；Windows Release 对照待完成。

## CoordinateMatrix 与 SciPy COO 独立对照

`export_coordinate_matrix` 从标准输入读取矩阵尺寸、原始条目数及逐行的 `row col value`，调用项目的 `insert()` 和 `canonicalize()`，然后以 CSV 输出实际结果。CSV 使用 `type,row,col,value` 四列：第一条 `shape` 记录矩阵行数、列数与整理后的 nnz；其余 `entry` 记录行号、列号和值。行列号使用整数读写，数值保留足够的 double 往返精度。

Python 脚本从同一份原始输入独立构造 SciPy `coo_matrix`，执行 `sum_duplicates()`、`eliminate_zeros()`，按行列排序参考结果，再逐项检查尺寸、nnz、坐标和值。对 C++ 输出不额外排序，确保能发现其顺序错误。全程使用稀疏三元组，不将大尺寸稀疏矩阵转换成稠密矩阵。

### Windows CMD

```bat
cmake --build --preset windows-debug --target export_coordinate_matrix
.venv\Scripts\python.exe python\validation\check_coordinate_matrix.py --exe out\build\windows-debug\export_coordinate_matrix.exe --output-dir out\validation\coordinate_matrix\windows-debug
```

也可以在 VS 中“生成 → 全部生成”，然后只执行 Python 命令。Release 对照将所有 `windows-debug` 替换为 `windows-release`。

### macOS 终端

```sh
cmake --build --preset macos-debug --target export_coordinate_matrix
./.venv/bin/python python/validation/check_coordinate_matrix.py --exe out/build/macos-debug/export_coordinate_matrix --output-dir out/validation/coordinate_matrix/macos-debug
```

14 组输入覆盖空形状、无序条目、多个重复项、完全抵消、中间和为零、显式零值、空行、超过 `2^53` 的大坐标及固定种子随机输入。重复项使用整数或可精确表示的小数，避免将不同浮点求和顺序混为坐标整理错误。

成功时显示 `CoordinateMatrix SciPy validation: PASS (14 cases)`。输出目录保存每组原始输入、`*_cpp.csv`、`*_scipy.csv` 和成功汇总 `summary.csv`。失败时脚本返回非零退出码并移除旧成功汇总。Windows Debug / Release 导出程序已编译并通过手算和大坐标 CSV 检查；Windows Debug 的 14 组 SciPy 对照已验收，结果位于 `out/validation/coordinate_matrix/windows-debug/`；macOS Debug / Release 的 14 组对照均已通过；Windows Release 对照待完成。

## CSR 转换与基础操作的 SciPy 独立对照

`export_compressed_sparse_row` 调用实际 C++ 实现，与 SciPy 对照 CSR 结构、SpMV、主对角线、无穷范数和对称性。26 组输入覆盖空形状、重复项、抵消、容差边界和固定种子随机矩阵。

### Windows CMD

```bat
cmake --build --preset windows-debug --target export_compressed_sparse_row
.venv\Scripts\python.exe python\validation\check_compressed_sparse_row.py --exe out\build\windows-debug\export_compressed_sparse_row.exe --output-dir out\validation\compressed_sparse_row\windows-debug
```

### macOS 终端

```sh
cmake --build --preset macos-debug --target export_compressed_sparse_row
./.venv/bin/python python/validation/check_compressed_sparse_row.py --exe out/build/macos-debug/export_compressed_sparse_row --output-dir out/validation/compressed_sparse_row/macos-debug
```

首次构建前先执行对应的 `cmake --preset`。Release 将命令中的 `debug` 替换为 `release`。

成功时显示 `CSR SciPy validation: PASS (26 cases)`，输入、C++ 结果、SciPy 结果和 `summary.csv` 保存在指定目录。macOS Debug / Release 各 26 组已通过；Windows 待验证。
