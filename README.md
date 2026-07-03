# ProgrammingDesign-FinalWork

PKU程序设计实习课程的大作业项目（from 码上搞定队）

## 📋 项目简介

这是一个**C/C++ 代码可视化教学平台**，旨在帮助学生直观理解程序执行过程。通过图形化界面展示代码的执行步骤、变量状态、内存分布、对象生命周期等，使抽象的编程概念更加具体化。

### 核心功能

- ✨ **代码执行可视化** - 逐步执行C/C++代码，实时显示执行流程
- 📊 **内存可视化** - 直观展示堆栈、对象、指针等内存结构
- 🏛️ **继承与多态支持** - 可视化展示类继承链、虚函数调用等面向对象特性
- 🤖 **AI教学模式** - 集成AI解释代码执行逻辑（需配置API密钥）
- 📚 **知识库管理** - 内置知识库和错题本，记录常见编程错误和解决方案

---

## 🏗️ 项目结构

```
ProgrammingDesign_FinalWork/
├── code_analysis/          # 核心分析引擎（C++）
│   ├── include/            # 头文件
│   ├── src/                # 源文件
│
├── ui/                     # Qt图形用户界面
│   ├── main.cpp            # 应用入口
│   ├── mainwindow.cpp/h    # 主窗口
│   ├── codeeditor.cpp/h    # 代码编辑器
│   ├── exevisualization.pro# Qt项目文件
│   └── resources/          # 图标和资源
│
├── knowledgebook/          # 知识库和错题本
│   ├── knowledgebookwidget.cpp/h  # 知识库UI组件
│   ├── markdownparser.cpp/h        # Markdown解析器
│   ├── KnowledgeBase/      # 知识库内容（Markdown格式）
│   └── ErrorNotebook/      # 常见错误记录
│
├── test_data/              # 测试用例
│   ├── *.c / *.cpp         # 各类型测试代码
│   └── debug_visualization/# 可视化调试数据
│
├── docs/                   # 作业文档
└── README.md               # 本文件
```

---

## 🔧 构建与编译

### 系统要求

- **Windows/Linux/macOS** with GCC/Clang (C++17)
- **Qt 5.x 或 Qt 6.x**（用于UI）
- **CMake** 或 **qmake**（用于构建）
- **可选**：CURL/WinHTTP（用于AI分析功能）

### 编译 code_analysis 模块

#### 编译 test_runner（独立测试）

**Windows (MinGW):**
```bash
g++ -std=c++17 -I code_analysis/include code_analysis/src/*.cpp -o code_analysis/bin/test_runner.exe -lwinhttp
```

**Linux/macOS:**
```bash
g++ -std=c++17 -I code_analysis/include code_analysis/src/*.cpp -o code_analysis/bin/test_runner -lcurl
```

#### 编译 Qt应用

```bash
cd ui
qmake exevisualization.pro
make  # 或 mingw32-make (Windows)
```

---

## 🚀 使用指南

### 运行应用

```bash
./ui/debug/exevisualization.exe  # Windows
./ui/exevisualization            # Linux/macOS
```

### 运行测试

```bash
# 扫描 test_data 目录中的所有 .c/.cpp 文件
./code_analysis/bin/test_runner.exe

# 详细输出（包含执行跟踪）
./code_analysis/bin/test_runner.exe -v

# 仅摘要输出
./code_analysis/bin/test_runner.exe -q

# 分析单个文件
./code_analysis/bin/test_runner.exe test_data/test_loop.c

# AI教学模式
./code_analysis/bin/test_runner.exe --mode ai --api-key sk-your-api-key
```

### 主窗口功能

| 功能 | 说明 |
|------|------|
| **代码输入** | 在左侧编辑器粘贴或输入C/C++代码 |
| **开始执行** | 点击"Start"按钮开始逐步执行 |
| **执行导航** | 使用"Next/Prev/First/Last"按钮在执行步骤间移动 |
| **执行跟踪** | 查看当前行、函数调用栈、局部变量值 |
| **内存可视化** | 实时展示堆对象、栈帧、指针关系 |
| **知识库** | 访问内置知识库和错题本 |

---

## 🔑 配置与扩展

### 启用AI教学模式

编辑代码或运行时指定：
```bash
./code_analysis/bin/test_runner.exe --mode ai --api-key sk-your-openai-api-key
```

需要在 `ai_analyzer.h` 中配置API端点。

### 添加新的测试用例

1. 在 `test_data/` 下创建 `.c` 或 `.cpp` 文件
2. 编写测试代码
3. 重新编译 test_runner
4. 运行：`./test_runner.exe test_data/your_file.cpp`

### 扩展知识库

编辑 `knowledgebook/KnowledgeBase/` 中的Markdown文件，或添加新文件。格式自动由 `markdownparser.cpp` 解析。

---

## 🤝 贡献指南

由于我们的项目没有使用许多现成的工具，所以一定有很多需要改进的地方。
欢迎有心人贡献代码、修复bug或改进文档。当然，请遵循以下步骤：

1. Fork 项目
2. 创建特性分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. Push 到分支 (`git push origin feature/AmazingFeature`)
5. 开启 Pull Request

谢谢！

---

**最后更新**: 2026年7月3日
