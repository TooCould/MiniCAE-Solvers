# MiniCAE-Solvers

使用 C++17 与 CMake 构建的小型数值代数学习项目。

## 当前进度

Phase 0：已建立最小 C++ 程序、CTest 启动测试、跨平台 CMake 预设和 Python 参考环境检查脚本。macOS 仍需实机验证，再进入数值模块开发。

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

在 Visual Studio 的 x64 Native Tools 命令提示符中执行。命令行还需可找到 CMake、CTest 和 Ninja；VS 的 C++ CMake 工具包含这些程序，VS 界面会负责设置构建环境。

```sh
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

构建产物分别保存在 `out/build/windows-debug/`、`out/build/windows-release/`、`out/build/macos-debug/` 和 `out/build/macos-release/`。
旧的 `out/build/x64-Debug/` 不受影响，但切换预设后不再使用它。生成文件不纳入版本控制，macOS 尚需实际编译验证。

如果 VS 仍只显示旧的 `x64-Debug` 配置，先关闭并重新打开项目文件夹；若仍未切换，在“工具 → 选项”中搜索 `CMake`，启用使用 `CMakePresets.json` 的选项，再重新打开文件夹。

## Python 独立验证环境

使用 Python 3.13.15。两台机器共用 `python/requirements.txt`，分别建立自己的 `.venv`，不复制或提交虚拟环境目录。以下命令均在项目根目录执行，直接调用环境中的 Python，无需激活环境。

### Windows PowerShell

```powershell
python -m venv .venv
.\.venv\Scripts\python.exe -m pip install --upgrade pip
.\.venv\Scripts\python.exe -m pip install -r python/requirements.txt
.\.venv\Scripts\python.exe python/validation/check_environment.py
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
