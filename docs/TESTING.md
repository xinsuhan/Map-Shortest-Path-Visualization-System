# 江安校区最短路径可视化系统测试指南

本文档用于验证项目能否正确配置、编译、运行，以及 Dijkstra、A*、地图读写和控制台交互是否符合预期。命令默认在项目根目录执行。

## 1. 测试目标

- 确认 CMake 能正确生成并编译项目。
- 确认全部自动化测试通过。
- 确认默认地图能加载 35 个节点和 73 条双向道路。
- 确认 Dijkstra 与 A* 能得到正确且一致的最短路径。
- 确认节点类型、尺寸以及新旧地图格式都能正确读取。
- 确认控制台菜单、起终点选择、动画速度和重置功能正常。
- 确认错误输入、缺失文件和不可达路径能被安全处理。

## 2. 测试前准备

打开 PowerShell，进入项目目录：

```powershell
cd C:\Users\17629\Desktop\Map-Shortest-Path-Visualization-System
```

检查 CMake：

```powershell
cmake --version
```

当前电脑已安装 CMake 4.3.2。项目要求 CMake 3.16 或更高版本。

检查 MinGW 编译器：

```powershell
& 'D:\new\Tools\mingw492_32\bin\gcc.exe' --version
& 'D:\new\Tools\mingw492_32\bin\mingw32-make.exe' --version
```

如果两条命令都能输出版本信息，即可继续。为当前 PowerShell 会话配置工具链：

```powershell
$env:PATH = 'D:\new\Tools\mingw492_32\bin;' + $env:PATH
```

此设置仅对当前终端有效，关闭 PowerShell 后不会修改系统环境变量。

## 3. 使用 CMake 配置测试构建

第一次测试时创建独立构建目录：

```powershell
cmake -S . -B build-test -G "MinGW Makefiles" -DCMAKE_C_COMPILER=gcc -DCMAKE_BUILD_TYPE=Debug
```

预期结果包含：

```text
Configuring done
Generating done
Build files have been written to: .../build-test
```

如果 `build-test` 已经配置完成，以后不必重复执行本节命令。

## 4. 编译项目

```powershell
cmake --build build-test
```

预期结果：所有目标成功构建，包括主程序和以下 8 个测试程序：

| 测试名称 | 主要验证内容 |
| --- | --- |
| `graph` | 节点、边、重复数据和邻接矩阵 |
| `dijkstra` | Dijkstra 最短路径 |
| `astar` | A* 最短路径与启发函数 |
| `storage` | CSV、map.txt、新旧节点格式和地图保存 |
| `scu_map` | 默认江安地图规模及代表性路线 |
| `input` | 整数和字符串输入处理 |
| `edge_cases` | 无效参数、孤立节点和不可达路径 |
| `path_result` | 路径结果的构造与处理 |

主程序生成位置：

```text
build-test\map_shortest_path.exe
```

## 5. 运行全部自动化测试

```powershell
ctest --test-dir build-test --output-on-failure
```

合格标准：

```text
100% tests passed, 0 tests failed out of 8
```

只要有一个测试失败，就不要提交代码。先记录失败测试名称和输出，再按第 7 节单独运行该测试。

## 6. 查看更详细的测试过程

显示每个测试执行的命令和完整输出：

```powershell
ctest --test-dir build-test -V
```

重复运行测试并显示失败原因：

```powershell
ctest --test-dir build-test --output-on-failure
```

## 7. 单独运行某个测试

通过 CTest 按名称运行：

```powershell
ctest --test-dir build-test -R storage --output-on-failure
ctest --test-dir build-test -R scu_map --output-on-failure
ctest --test-dir build-test -R "dijkstra|astar" --output-on-failure
```

也可以直接运行测试程序：

```powershell
.\build-test\test_graph.exe
.\build-test\test_dijkstra.exe
.\build-test\test_astar.exe
.\build-test\test_storage.exe
.\build-test\test_scu_map.exe
.\build-test\test_input.exe
.\build-test\test_edge_cases.exe
.\build-test\test_path_result.exe
```

测试成功时通常会输出类似：

```text
test_storage passed
```

## 8. 默认地图专项测试

默认地图文件是 `data/map.txt`。首先检查文件声明：

```powershell
Select-String -Path data\map.txt -Pattern '^NODES ','^EDGES '
```

预期结果：

```text
NODES 35
EDGES 73
```

运行地图测试：

```powershell
ctest --test-dir build-test -R scu_map --output-on-failure
```

此测试会验证以下代表性结果：

1. 从 `XiyuanDorm`（1）到 `EastGate`（13）：

```text
1 -> 0 -> 27 -> 30 -> 31 -> 32 -> 13
Total distance: 12.8
```

2. 从 `WestSportsField`（4）到 `AdministrationBuilding`（10）：

```text
4 -> 27 -> 28 -> 29 -> 7 -> 10
Total distance: 10.3
```

Dijkstra 和 A* 都应得到上述路径和距离。

## 9. 手工功能测试

启动程序：

```powershell
.\build-test\map_shortest_path.exe
```

按照下表逐项操作并记录结果：

| 编号 | 操作 | 预期结果 |
| --- | --- | --- |
| M01 | 启动程序 | 显示江安校区字符地图、节点列表和主菜单 |
| M02 | 输入 `1`，选择起点 `1` | 起点显示为 `XiyuanDorm`，地图上使用绿色标记 |
| M03 | 输入 `2`，选择终点 `13` | 终点显示为 `EastGate`，地图上使用红色标记 |
| M04 | 输入 `3`，选择 `1` | 当前算法显示为 Dijkstra |
| M05 | 输入 `4`，选择 `3` | 动画速度显示为 Fast（150 ms/frame） |
| M06 | 输入 `5` | 播放节点访问动画，结束后高亮最短路径 |
| M07 | 查看结果 | 总距离为 `12.80`，路径与第 8 节一致 |
| M08 | 将算法切换为 A* 后再次运行 | 最短距离和路径与 Dijkstra 一致 |
| M09 | 输入 `7` | 显示 35 个节点、73 条边及节点类型 |
| M10 | 输入 `6` | 起点、终点和速度恢复默认值 |
| M11 | 输入 `0` | 程序正常退出并显示 `Goodbye.` |

## 10. 异常输入测试

重新启动程序，分别验证：

| 编号 | 输入或操作 | 预期结果 |
| --- | --- | --- |
| E01 | 主菜单输入字母 `abc` | 提示输入无效，程序不崩溃 |
| E02 | 主菜单输入 `9` | 提示未知选项 |
| E03 | 起点输入不存在的 ID `999` | 提示节点不存在 |
| E04 | 起点和终点选择同一 ID | 提示起点和终点必须不同 |
| E05 | 未选择起终点就开始可视化 | 提示先选择起点和终点 |
| E06 | 算法选择输入 `3` | 提示算法选择无效 |
| E07 | 速度选择输入 `0` 或 `4` | 提示速度选择无效 |

每次异常输入后，程序都应继续显示菜单，不能闪退、卡死或产生乱码。

## 11. 自定义地图文件测试

使用测试地图启动：

```powershell
.\build-test\map_shortest_path.exe tests\test_data\map.txt
```

该地图包含三种节点：

- `A`：`GATE`
- `B`：`JUNCTION`
- `C`：`LANDMARK`

预期结果：地图成功加载；路口节点 `B` 在字符地图中显示为 `+`，并能参与最短路径计算。

测试旧版四字段 CSV 格式：

```powershell
.\build-test\map_shortest_path.exe tests\test_data\nodes.csv tests\test_data\edges.csv
```

预期结果：旧格式仍能正常加载，未提供类型的节点自动按 `LANDMARK` 处理。

测试缺失文件：

```powershell
.\build-test\map_shortest_path.exe missing-map.txt
```

预期结果：程序输出地图加载失败信息并以非零状态退出，不发生崩溃。

## 12. 修改代码后的回归测试流程

每次修改代码后依次执行：

```powershell
$env:PATH = 'D:\new\Tools\mingw492_32\bin;' + $env:PATH
cmake --build build-test
ctest --test-dir build-test --output-on-failure
git diff --check
git status --short
```

提交代码前必须满足：

- 编译无错误。
- 8 个自动化测试全部通过。
- `git diff --check` 没有空白错误。
- `git status --short` 中只包含本次计划提交的源文件或文档。
- 至少完成 M01～M11 的手工冒烟测试。

### 3D 模式测试

配置并编译启用 raylib 的 3D 版本：

```powershell
$env:PATH = 'D:\new\Tools\mingw492_32\bin;' + $env:PATH
cmake -S . -B build-3d -G "MinGW Makefiles" -DCMAKE_C_COMPILER=gcc -DCMAKE_BUILD_TYPE=Debug -DMSP_ENABLE_3D=ON
cmake --build build-3d
ctest --test-dir build-3d --output-on-failure
.\build-3d\map_shortest_path.exe
```

在控制台选择 `8`，检查以下项目：

- 建筑保持低矮，不遮挡道路。
- 湖泊、广场、校门、建筑和路口形态不同。
- 普通道路为浅灰细线，最终路径为亮色粗线。
- 左键和右键能分别选择起点、终点。
- `Enter` 能启动访问动画，右侧显示距离和访问节点数。
- `D` 与 `A` 的最短距离一致。
- `WASD`、滚轮、`Home` 和 `R` 工作正常。
- `Esc` 能关闭 3D 窗口并返回控制台。

## 13. 如何新增测试代码

项目使用 C 标准库 `assert` 编写单元测试。新增测试时，在 `tests` 目录创建一个独立的 `.c` 文件。

例如创建 `tests/test_node_types.c`：

```c
#include "graph.h"

#include <assert.h>
#include <stdio.h>

int main(void) {
    Graph graph;
    Node junction = {
        100,
        "TestJunction",
        5.0,
        6.0,
        NODE_JUNCTION,
        0.15f,
        0.15f,
        0.03f
    };

    graph_init(&graph);
    assert(graph_add_node(&graph, junction) == MSP_OK);
    assert(graph.node_count == 1);
    assert(graph.nodes[0].type == NODE_JUNCTION);
    assert(graph.nodes[0].id == 100);

    puts("test_node_types passed");
    return 0;
}
```

然后打开 `CMakeLists.txt`，在测试名称列表末尾加入 `node_types`：

```cmake
foreach(test_name graph dijkstra astar storage scu_map input edge_cases path_result node_types)
```

CMake 会自动完成以下工作：

- 编译 `tests/test_node_types.c`。
- 生成 `test_node_types.exe`。
- 链接 `shortest_path_core`。
- 注册名为 `node_types` 的 CTest 测试。

重新配置、编译并运行新测试：

```powershell
cmake -S . -B build-test
cmake --build build-test
ctest --test-dir build-test -R node_types --output-on-failure
ctest --test-dir build-test --output-on-failure
```

编写测试代码时遵守以下规则：

- 每个测试文件只负责一类功能。
- 正常输入、边界输入和错误输入都要覆盖。
- 测试结果必须可重复，不能依赖动画速度或人工输入。
- 测试生成的临时文件必须在结束前删除。
- 浮点数不要直接用 `==` 比较，应使用误差范围。
- 修复缺陷时，先写一个能复现缺陷的测试，再修改实现。

浮点数测试示例：

```c
#include <math.h>

assert(fabs(result.total_distance - 12.8) < MSP_EPSILON);
```

## 14. 常见问题

### CMake 提示找不到 nmake

原因是构建目录缓存了 NMake 生成器，但当前终端没有 MSVC 环境。不要继续使用该目录，改用新的 MinGW 构建目录：

```powershell
$env:PATH = 'D:\new\Tools\mingw492_32\bin;' + $env:PATH
cmake -S . -B build-test -G "MinGW Makefiles" -DCMAKE_C_COMPILER=gcc -DCMAKE_BUILD_TYPE=Debug
```

### gcc 能输出版本，但 CMake 编译器检测失败

先确认 MinGW 的完整 `bin` 目录位于当前 PATH 最前面：

```powershell
$env:PATH = 'D:\new\Tools\mingw492_32\bin;' + $env:PATH
Get-Command gcc
Get-Command mingw32-make
```

两条命令都应指向 `D:\new\Tools\mingw492_32\bin`。

### CTest 显示 No tests were found

通常说明 CMake 配置没有成功，或使用了错误的构建目录。重新执行第 3 节，再运行：

```powershell
ctest --test-dir build-test -N
```

预期列出 8 个测试。

### 控制台颜色或中文显示异常

建议使用 Windows Terminal 或 VS Code PowerShell，并确保终端使用 UTF-8。程序启动时会自动尝试启用 ANSI 颜色和 UTF-8 输出。

## 15. 测试记录模板

完成测试后可复制以下模板填写：

```text
项目：Map Shortest Path Visualization System
测试日期：
测试人员：
操作系统：Windows
CMake 版本：
GCC 版本：

配置结果：通过 / 失败
编译结果：通过 / 失败
自动化测试：通过数量 ____ / 8
手工功能测试：通过数量 ____ / 11
异常输入测试：通过数量 ____ / 7

失败项目：
1.
2.

错误输出：

结论：可以提交 / 修复后重新测试
```

## 16. 测试通过后的 Git 提交

确认测试全部通过后执行：

```powershell
git status
git add README.md data include src tests docs\TESTING.md
git commit -m "feat: add node metadata and testing guide"
git push
```

如果首次推送当前分支：

```powershell
git push -u origin HEAD
```
