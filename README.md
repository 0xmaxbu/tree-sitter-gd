# tree-sitter-gd

基于 tree-sitter 的 GDScript AST 分析工具，以 Godot 4.3 GDExtension 形式提供。

## 项目简介

`tree-sitter-gd` 是一个 Godot 插件，它将强大的 [tree-sitter](https://tree-sitter.github.io/) 增量解析器集成到 Godot Engine 中，为 GDScript 提供高性能的抽象语法树（AST）分析能力。

本项目使用 C++ 实现 GDExtension，可在 Godot 4.3+ 中直接调用，适用于：
- 代码自动补全和智能提示
- 代码格式化和重构工具
- 静态代码分析和 Lint 工具
- 语法高亮和语义分析
- AI 辅助编程工具

## 核心功能

### 基础功能
- ✅ **健康检查**：`ping()` 方法验证扩展加载状态
- ✅ **版本查询**：`get_version()` 获取插件版本信息
- ✅ **语法解析**：`parse_test()` 测试代码解析能力

### 文件管理
- ✅ **文件缓存**：`open_file()` 打开文件并缓存 AST
- ✅ **状态查询**：`is_file_open()` 检查文件是否已打开
- ✅ **内容获取**：`get_file_source()` 获取文件源码
- ✅ **文件更新**：`update_file()` 更新文件内容并重新解析
- ✅ **批量管理**：`get_open_files()` 列出所有打开的文件

### AST 查询
- ✅ **Tree-sitter 查询**：`query()` 使用 tree-sitter 查询语法搜索 AST 节点
- ✅ **节点文本提取**：`get_node_text()` 根据字节范围提取节点文本
- ✅ **S表达式导出**：`get_sexp()` 导出整棵语法树的 S 表达式

示例查询：
```gdscript
var ast = ASTManager.new()
ast.open_file("player.gd", source_code)

# 查找所有函数定义
var result = ast.query("player.gd", "(function_definition) @func")
for match in result["matches"]:
    print("Found function: ", match["captures"][0]["text"])
```

### 文本编辑
- ✅ **字节级编辑**：`apply_text_edits()` 精确的字节级文本替换
- ✅ **批量编辑**：支持多个编辑操作原子性执行
- ✅ **干运行模式**：`dry_run=true` 预览编辑结果而不实际修改

### AST 节点编辑
- ✅ **节点级编辑**：`apply_node_edits()` 基于 AST 节点的语义编辑
- ✅ **自动缩进**：智能处理缩进，保持代码风格一致
- ✅ **安全模式**：编辑后自动验证语法正确性

### 代码差异与验证
- ✅ **统一差异格式**：`generate_diff()` 生成 unified diff 格式的代码对比
- ✅ **语法验证**：`validate()` 检查 GDScript 代码是否有语法错误

### CI/CD 自动构建
- ✅ **多平台支持**：自动构建 Windows x86_64、Linux x86_64、macOS Universal 二进制库
- ✅ **GitHub Actions**：推送代码自动触发构建和测试
- ✅ **自动发布**：推送 tag 自动创建 GitHub Release 并附带编译好的库

## 系统要求

### 运行环境
- **Godot Engine**: 4.3 或更高版本
- **操作系统**:
  - Windows 10/11 (x86_64)
  - Linux (x86_64, glibc 2.31+)
  - macOS 10.15+ (x86_64 + Apple Silicon 通用二进制)

### 构建环境
- **编译器**:
  - Windows: MSVC 2019+ 或 Clang
  - Linux: GCC 9+ 或 Clang 10+
  - macOS: Xcode 12+ (Command Line Tools)
- **构建工具**:
  - Python 3.6+
  - SCons 4.0+
- **依赖库（已包含在子模块中）**:
  - godot-cpp (branch: godot-4.3-stable)
  - tree-sitter (核心解析器)
  - tree-sitter-gdscript (GDScript 语法定义)

## 安装方法

### 方式 1：从 GitHub Releases 下载（推荐）

1. 访问 [Releases 页面](https://github.com/0xmaxbu/tree-sitter-gd/releases)
2. 下载最新版本的 `ai_script_plugin.zip`
3. 解压到你的 Godot 项目根目录，确保文件结构如下：
   ```
   你的项目/
   └── addons/
       └── ai_script_plugin/
           ├── ast.gdextension
           ├── plugin.cfg
           ├── plugin.gd
           └── bin/
               ├── ast.windows.x86_64.dll       (Windows)
               ├── libast.linux.x86_64.so        (Linux)
               └── libast.macos.universal.dylib  (macOS)
   ```
4. 在 Godot 编辑器中：`项目 -> 项目设置 -> 插件`，启用 "AI Script Plugin"

### 方式 2：从 GitHub Actions 下载 Artifacts

如果你需要最新的开发版本：

1. 访问 [Actions 页面](https://github.com/0xmaxbu/tree-sitter-gd/actions)
2. 点击最近一次成功的 workflow（绿色 ✓）
3. 滚动到底部，下载对应平台的 artifact：
   - `windows-x86_64.zip`
   - `linux-x86_64.zip`
   - `macos-universal.zip`
4. 解压并放置到 `addons/ai_script_plugin/bin/` 目录
5. 确保 `ast.gdextension` 文件存在并配置正确

### 方式 3：从源码构建

参见下方的"构建方法"章节。

## 使用方法

### 快速开始

```gdscript
extends Node

func _ready():
    var ast = ASTManager.new()
    
    # 1. 基础测试
    print(ast.ping())  # 输出: "pong"
    print(ast.get_version())  # 输出: "0.1.0"
    
    # 2. 解析 GDScript 代码
    var code = """
    func calculate(a: int, b: int) -> int:
        return a + b
    """
    var result = ast.parse_test(code)
    if result["success"]:
        print("解析成功！")
    
    # 3. 打开文件进行分析
    ast.open_file("player.gd", code)
    print("文件是否打开: ", ast.is_file_open("player.gd"))
    
    # 4. 查询 AST
    var query_result = ast.query("player.gd", "(function_definition) @func")
    print("找到函数数量: ", query_result["matches"].size())
    
    # 5. 关闭文件
    ast.close_file("player.gd")
```

### 高级用例

#### 查找所有类成员变量
```gdscript
var ast = ASTManager.new()
ast.open_file("character.gd", source_code)

var query_str = """
(variable_statement
  (assignment
    (identifier) @var_name))
"""
var result = ast.query("character.gd", query_str)

for match in result["matches"]:
    for capture in match["captures"]:
        if capture["name"] == "var_name":
            print("变量名: ", capture["text"])
```

#### 批量重命名变量
```gdscript
var ast = ASTManager.new()
ast.open_file("script.gd", source_code)

# 查找所有 old_name 变量的位置
var query_result = ast.query("script.gd", "(identifier) @id")

var edits = []
for match in query_result["matches"]:
    for capture in match["captures"]:
        if capture["text"] == "old_name":
            edits.append({
                "start_byte": capture["start_byte"],
                "end_byte": capture["end_byte"],
                "new_text": "new_name"
            })

# 应用编辑（自动按倒序排序）
var result = ast.apply_text_edits("script.gd", edits, false)
if result["success"]:
    print("重命名成功！")
    print("新代码:\n", result["new_content"])
```

#### 代码格式化：添加类型注解
```gdscript
var ast = ASTManager.new()
ast.open_file("untyped.gd", source_code)

# 查找未添加类型注解的变量
var query_str = """
(variable_statement
  (assignment
    left: (identifier) @var_name
    right: (_) @value))
"""
var result = ast.query("untyped.gd", query_str)

var edits = []
for match in result["matches"]:
    var var_node = match["captures"][0]
    # 在变量名后插入类型注解
    edits.append({
        "start_byte": var_node["end_byte"],
        "end_byte": var_node["end_byte"],
        "new_text": ": int"  # 假设是 int 类型
    })

ast.apply_text_edits("untyped.gd", edits, false)
```

#### 语法检查
```gdscript
var ast = ASTManager.new()

var code = """
func broken_function(:
    print("missing parameter"
"""

var result = ast.validate(code)
if not result["valid"]:
    print("语法错误！")
    print("错误信息: ", result["error"])
else:
    print("代码语法正确")
```

#### 生成代码差异对比
```gdscript
var ast = ASTManager.new()

var old_code = "func foo():\n\tprint('old')\n"
var new_code = "func foo():\n\tprint('new')\n"

var diff = ast.generate_diff(old_code, new_code, "script.gd")
print(diff)

# 输出示例:
# --- a/script.gd
# +++ b/script.gd
# @@ -1,2 +1,2 @@
#  func foo():
# -    print('old')
# +    print('new')
```

## 构建方法

### 1. 克隆仓库

```bash
git clone --recursive https://github.com/0xmaxbu/tree-sitter-gd.git
cd tree-sitter-gd
```

如果忘记 `--recursive`，可以补充初始化子模块：
```bash
git submodule update --init --recursive
```

### 2. 安装构建依赖

#### Windows
```powershell
# 安装 Python 和 SCons
pip install scons

# 安装 Visual Studio 2019+ 或 Build Tools
# 下载地址: https://visualstudio.microsoft.com/downloads/
```

#### Linux
```bash
# Ubuntu/Debian
sudo apt-get install build-essential scons python3

# Fedora/RHEL
sudo dnf install gcc-c++ scons python3
```

#### macOS
```bash
# 安装 Xcode Command Line Tools
xcode-select --install

# 安装 SCons
pip3 install scons
```

### 3. 构建 godot-cpp

```bash
cd thirdparty/godot-cpp

# Windows (使用 MSVC)
scons platform=windows target=template_release

# Linux
scons platform=linux target=template_release

# macOS (Universal Binary)
scons platform=macos target=template_release arch=universal

cd ../..
```

### 4. 构建 tree-sitter

```bash
cd thirdparty/tree-sitter
make
cd ../..
```

### 5. 构建 GDExtension

```bash
# Windows
scons platform=windows target=template_release

# Linux
scons platform=linux target=template_release

# macOS
scons platform=macos target=template_release arch=universal
```

构建完成后，库文件将输出到：
```
addons/ai_script_plugin/bin/
├── libast.windows.template_release.x86_64.dll
├── libast.linux.template_release.x86_64.so
└── libast.macos.template_release.framework/
    └── libast.macos.universal.dylib
```

### 6. 验证构建

#### 方法 A：使用快速测试脚本

```bash
# 复制对应平台的测试配置
cp test/phase8_quick_tests/ast.gdextension.macos addons/ai_script_plugin/ast.gdextension  # macOS
# 或
cp test/phase8_quick_tests/ast.gdextension.linux addons/ai_script_plugin/ast.gdextension  # Linux
# 或
cp test/phase8_quick_tests/ast.gdextension.windows addons/ai_script_plugin/ast.gdextension  # Windows

# 运行测试（macOS/Linux）
godot --headless --path . --script test/phase8_quick_tests/test_macos.gd
godot --headless --path . --script test/phase8_quick_tests/test_linux.gd

# 运行测试（Windows PowerShell）
& "C:\Program Files\Godot\Godot.exe" --headless --path . --script test/phase8_quick_tests/test_windows.gd
```

**期望输出**：
```
=== macOS Library Test ===

[Test 1/9] Testing ping()...
✅ PASS

[Test 2/9] Testing get_version()...
✅ PASS

...

==================================================
🎉 ALL TESTS PASSED (9/9)
==================================================
```

#### 方法 B：在 Godot 编辑器中测试

1. 用 Godot 打开项目目录
2. 启用插件：`项目 -> 项目设置 -> 插件 -> AI Script Plugin`
3. 创建测试脚本 `test.gd`：
   ```gdscript
   extends Node
   
   func _ready():
       var ast = ASTManager.new()
       assert(ast.ping() == "pong", "ASTManager failed to load!")
       print("✅ ASTManager loaded successfully!")
       print("Version: ", ast.get_version())
   ```
4. 运行场景，查看输出

### 构建选项

```bash
# 构建 debug 版本（包含调试符号）
scons platform=linux target=template_debug

# 指定架构（32位）
scons platform=windows target=template_release arch=x86_32

# 清理构建产物
scons --clean

# 查看所有可用选项
scons --help
```

### 常见构建问题

#### Windows: 找不到 MSVC 编译器
```powershell
# 手动指定 MSVC 工具链路径
scons platform=windows use_mingw=False vcvars="C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
```

#### Linux: 缺少 Python.h
```bash
sudo apt-get install python3-dev
```

#### macOS: 无效的 SDK 路径
```bash
# 重新安装 Command Line Tools
sudo rm -rf /Library/Developer/CommandLineTools
xcode-select --install
```

#### 链接错误：找不到 libgodot-cpp
确保已先构建 godot-cpp：
```bash
cd thirdparty/godot-cpp
scons platform=<your_platform> target=template_release
```

## 开发与测试

### 运行测试套件

项目包含完整的测试套件，覆盖 Phases 1-7 的所有功能：

```bash
# 快速测试（推荐）
godot --headless --path . --script test/phase8_quick_tests/test_macos.gd

# 完整测试套件（多个测试文件）
for test in test/*.gd; do
    godot --headless --path . --script "$test"
done
```

### 代码风格

本项目遵循以下代码规范：

#### C++ 代码
- 格式化工具：clang-format（配置来自 godot-cpp）
- 缩进：Tab（宽度 4）
- 命名：
  - 类名：PascalCase
  - 函数/变量：snake_case
  - 常量：UPPER_SNAKE_CASE

#### GDScript 代码
- 缩进：Tab
- 类型注解：必需（所有变量和函数返回值）
- 命名：
  - 函数/变量：snake_case
  - 常量：UPPER_SNAKE_CASE
  - 私有成员：`_prefix`

### 格式化代码

```bash
# C++
cd thirdparty/godot-cpp
find ../../src -name "*.cpp" -o -name "*.h" | xargs clang-format -i

# GDScript（需要 gdformat 或 gdtoolkit）
pip install gdtoolkit
gdformat test/*.gd
```

## 项目架构

```
tree-sitter-gd/
├── addons/ai_script_plugin/      # Godot 插件目录
│   ├── ast.gdextension           # GDExtension 配置
│   ├── plugin.cfg                # 插件元数据
│   ├── plugin.gd                 # 插件入口脚本
│   └── bin/                      # 编译后的动态库
├── src/                          # C++ 源代码
│   ├── ast_manager.h/cpp         # ASTManager 类实现
│   └── register_types.h/cpp      # GDExtension 注册代码
├── test/                         # 测试文件
│   ├── phase8_quick_tests/       # 快速测试脚本
│   └── gdscript_samples/         # GDScript 示例代码
├── thirdparty/                   # 第三方依赖（子模块）
│   ├── godot-cpp/                # Godot C++ 绑定
│   ├── tree-sitter/              # tree-sitter 核心
│   └── tree-sitter-gdscript/     # GDScript 语法定义
├── docs/                         # 开发文档
│   └── Phase8_Testing_Manual.md  # 测试手册
├── .github/workflows/            # CI/CD 配置
│   └── build.yml                 # 自动构建工作流
├── SConstruct                    # SCons 构建脚本
└── AGENTS.md                     # AI 开发代理指南
```

## API 参考

详细 API 文档请参见 `src/ast_manager.h`。以下是主要方法签名：

```cpp
// 基础方法
String ping();
String get_version();
Dictionary parse_test(const String &source_code);

// 文件管理
Dictionary open_file(const String &file_path, const String &content);
bool close_file(const String &file_path);
Dictionary update_file(const String &file_path, const String &new_content);
bool is_file_open(const String &file_path);
PackedStringArray get_open_files();
String get_file_source(const String &file_path);

// AST 查询
Dictionary query(const String &file_path, const String &query_string);
String get_node_text(const String &file_path, int start_byte, int end_byte);
String get_sexp(const String &file_path);

// 文本编辑
Dictionary apply_text_edits(const String &file_path, const TypedArray<Dictionary> &edits, bool dry_run);
Dictionary apply_node_edits(const String &file_path, const TypedArray<Dictionary> &edits, const Dictionary &options);

// 代码分析
String generate_diff(const String &old_text, const String &new_text, const String &file_name);
Dictionary validate(const String &source_code);
```

### 返回值结构

大多数方法返回 Dictionary，包含以下字段：

```gdscript
{
    "success": bool,        # 操作是否成功
    "error": String,        # 错误信息（如果失败）
    "matches": Array,       # query() 返回的匹配结果
    "new_content": String,  # apply_*_edits() 返回的新内容
    "valid": bool,          # validate() 返回的有效性
    ...                     # 其他方法特定字段
}
```

## CI/CD

本项目使用 GitHub Actions 自动构建三平台库：

- **触发条件**：
  - 推送代码到任何分支
  - 创建 Pull Request
  - 推送 Git Tag（额外触发 Release 创建）

- **构建产物**：
  - Windows x86_64 DLL
  - Linux x86_64 SO
  - macOS Universal Dylib (x86_64 + arm64)

- **下载方式**：
  - 从 Actions 页面下载 Artifacts（90 天有效期）
  - 从 Releases 页面下载完整 ZIP（永久）

详细的 CI/CD 工作流配置见 `.github/workflows/build.yml`。

## 常见问题

### Q: Godot 无法加载扩展，报 "Extension not found"
**A**: 检查以下几点：
1. 确认 `ast.gdextension` 文件存在于 `addons/ai_script_plugin/` 目录
2. 检查 `[libraries]` 部分的路径是否正确
3. 确认对应平台的动态库文件存在于 `bin/` 目录
4. 在 Godot 中重新加载项目（`项目 -> 重新加载当前项目`）

### Q: Windows 下报 "找不到 DLL"
**A**: 可能缺少 Visual C++ Redistributable：
- 下载安装：https://aka.ms/vs/17/release/vc_redist.x64.exe

### Q: Linux 下报 "GLIBC version not found"
**A**: 系统 glibc 版本过旧，需要：
1. 升级系统到较新版本（推荐 Ubuntu 20.04+）
2. 或从源码重新构建（使用本地 glibc）

### Q: macOS 下报 "无法验证开发者"
**A**: 苹果的安全限制，执行以下命令：
```bash
xattr -cr addons/ai_script_plugin/bin/libast.macos.universal.dylib
```

### Q: 查询时报 "Query syntax error"
**A**: tree-sitter 查询语法错误，参考：
- [Tree-sitter 查询语法](https://tree-sitter.github.io/tree-sitter/using-parsers#pattern-matching-with-queries)
- [GDScript 语法节点类型](https://github.com/PrestonKnopp/tree-sitter-gdscript/blob/main/src/node-types.json)

### Q: `apply_text_edits()` 失败
**A**: 常见原因：
1. 字节偏移量错误（GDScript 使用 UTF-8，一个字符可能占多个字节）
2. 编辑范围重叠（需要确保编辑操作不冲突）
3. 文件未打开（先调用 `open_file()`）

## 性能说明

- **解析性能**: tree-sitter 是增量解析器，只重新解析修改的部分，适合实时分析
- **内存占用**: 每个打开的文件占用约 1-5MB 内存（取决于文件大小）
- **查询性能**: 简单查询通常在 1-10ms 内完成（1000 行代码）

**建议**:
- 不使用的文件及时 `close_file()` 释放内存
- 频繁修改的文件保持打开状态，利用增量解析优势
- 复杂查询（深层嵌套）可能耗时较长，考虑异步处理

## 许可证

本项目使用 MIT 许可证。详见 LICENSE 文件。

依赖库许可证：
- godot-cpp: MIT License
- tree-sitter: MIT License
- tree-sitter-gdscript: MIT License

## 致谢

- [tree-sitter](https://tree-sitter.github.io/) - 强大的增量解析器
- [Godot Engine](https://godotengine.org/) - 开源游戏引擎
- [tree-sitter-gdscript](https://github.com/PrestonKnopp/tree-sitter-gdscript) - GDScript 语法定义

## 联系方式

- GitHub Issues: https://github.com/0xmaxbu/tree-sitter-gd/issues
- 项目主页: https://github.com/0xmaxbu/tree-sitter-gd

---

**如果这个项目对你有帮助，请给个 Star ⭐️！**
