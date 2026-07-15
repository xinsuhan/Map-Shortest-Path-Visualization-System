# Map Shortest Path Visualization System

## 项目简介

**Map Shortest Path Visualization System** 是一个地图最短路径可视化系统，主要用于展示图结构中最短路径算法的运行过程。

本项目支持 Dijkstra 算法和 A* 算法。当前版本默认加载四川大学江安校区曲线路网增强数据，提供终端菜单与可选 3D 校园导航界面；道路和推荐路线都会沿折线控制点绘制，不再用端点直线代替实际转弯。

该项目适合作为数据结构、算法设计、图论、人工智能搜索算法或课程设计项目。

---

## 项目功能

- 地图 / 图结构可视化展示
- 支持节点、道路和边权的字符化显示
- 支持选择起点和终点
- 支持 Dijkstra 最短路径算法
- 支持 A* 启发式搜索算法
- 支持高亮显示最终最短路径
- 支持显示路径经过的节点
- 支持显示路径总距离或总代价
- 支持对比不同算法的搜索效果
- 支持新版底图的 86 个道路/入口节点、38 个地点和 297 个道路控制点
- 支持按曲线长度（米）计算最短路和步行时间
- 便于直观理解最短路径搜索过程

---

## 项目特点

### 1. 算法可视化

传统最短路径算法通常只输出结果，而本项目通过控制台可视化界面展示搜索过程，使算法执行过程更加直观。

用户可以清楚看到：

- 哪些节点被访问过
- 当前正在搜索哪个节点
- 最终路径经过哪些节点
- 不同算法的搜索范围差异

### 2. 支持 Dijkstra 算法

Dijkstra 算法适用于边权非负的图，可以保证找到从起点到终点的最短路径。

其核心思想是：

1. 将起点距离设为 0。
2. 每次选择当前距离最小的未访问节点。
3. 更新该节点相邻节点的最短距离。
4. 重复以上过程，直到找到终点。

### 3. 支持 A* 算法

A* 算法是一种启发式搜索算法，常用于地图导航和游戏寻路。

A* 使用如下估价函数：

```text
f(n) = g(n) + h(n)
```

其中：

- `g(n)` 表示从起点到当前节点的实际代价
- `h(n)` 表示从当前节点到终点的估计代价
- `f(n)` 表示当前节点的综合评估代价

相比 Dijkstra 算法，A* 算法在地图场景中通常搜索范围更小，效率更高。程序会根据道路权值与坐标距离自动缩放启发函数，避免地图逻辑坐标和演示权值比例不一致时高估剩余代价。

---

## 技术栈

- 编程语言：C11
- 构建工具：CMake 3.16+
- 测试工具：CTest、C 标准库 `assert`
- 可选 3D 渲染：raylib 5.5（CMake 自动下载）
- 核心算法：Dijkstra Algorithm、A* Search Algorithm
- 核心数据结构：
  - Graph 图
  - Node 节点
  - Edge 边
  - Array 数组
  - Adjacency Matrix 邻接矩阵
- 数据存储：曲线路网 CSV（默认）以及兼容的 `map.txt` / 旧版 CSV
- 可视化展示：原生控制台字符地图、ANSI 状态颜色和搜索动画

---

## 项目结构

```text
Map-Shortest-Path-Visualization-System/
│
├── CMakeLists.txt            # CMake 构建配置
├── LICENSE                   # MIT 开源许可证
├── include/                  # 公共接口与数据模型
│   ├── graph.h
│   ├── dijkstra.h
│   ├── astar.h
│   ├── storage.h
│   ├── input.h
│   ├── renderer3d.h
│   └── visualization.h
│
├── src/                      # C 源代码
├── data/                     # 川大江安校区简化地图数据
├── tests/                    # 单元测试与测试数据
├── docs/                     # 项目全过程文档
├── assets/                   # 图表与运行截图
├── demo/                     # 演示数据和演示脚本
└── README.md                 # 项目说明文档
```

---

## 安装与运行

### 1. 克隆仓库

```bash
git clone https://github.com/xinsuhan/Map-Shortest-Path-Visualization-System.git
cd Map-Shortest-Path-Visualization-System
```

### 2. Windows + VS Code 环境准备

VS Code 只是代码编辑器，本身不包含 C/C++ 编译器。Windows 用户推荐安装 **Visual Studio Build Tools 2022**，并在 Visual Studio Installer 中选择 **Desktop development with C++（使用 C++ 的桌面开发）**，确认包含以下组件：

- MSVC v143
- Windows 10 SDK 或 Windows 11 SDK
- C++ CMake tools for Windows

VS Code 中建议同时安装 Microsoft 提供的 **C/C++** 和 **CMake Tools** 扩展。安装完成后，请重新打开 VS Code 和终端。

可以在 **Developer PowerShell for VS 2022** 中输入以下命令检查编译器：

```powershell
cl
```

如果提示 `cl` 不是内部或外部命令，说明 MSVC 尚未正确安装，或者安装后还没有重新打开 VS Code/终端。也可以直接使用 Developer PowerShell for VS 2022 打开项目，以加载 MSVC 环境。

### 3. Windows 编译与运行

推荐使用 Visual Studio Build Tools 2022：

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
.\build\Debug\map_shortest_path.exe
```

也可以使用 MinGW。请先确保 `gcc`、`mingw32-make` 和 `cmake` 均已加入 `PATH`：

```powershell
cmake -S . -B build -G "MinGW Makefiles" -DMSP_ENABLE_3D=OFF
cmake --build build
ctest --test-dir build --output-on-failure
.\build\map_shortest_path.exe
```

### 4. Linux / macOS 编译与运行

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
./build/map_shortest_path
```

### 5. Windows 3D 模式

3D 模式使用 raylib 5.5。首次配置时 CMake 会从 raylib 官方仓库下载并编译依赖：

```powershell
cmake -S . -B build-3d -DMSP_ENABLE_3D=ON
cmake --build build-3d
.\build-3d\map_shortest_path.exe
```

启动后在控制台菜单选择 `8` 进入 3D 校园视图，也可以直接启动：

```powershell
.\build-3d\map_shortest_path.exe --3d
```

3D 模式保留原控制台功能，并提供：

- 左侧导航面板、顶部地点搜索、右侧图例和地图工具
- 低矮 2.5D 功能建筑、校门、湖泊、广场、运动场、绿地和树木
- 按 `geometry` 控制点逐段绘制的分级道路面与橙色推荐路径
- 左键选择起点、右键选择终点
- `D` / `A` 切换 Dijkstra 与 A*
- `Enter` 开始搜索，`Space` 暂停或继续动画
- `1` / `2` / `3` 调整动画速度
- `F1` / `F2` / `F3` 切换导航、全览和路径视角
- 右下角按钮支持缩放、复位和 2.5D/3D 展示切换
- 在地图空白处按住左键或任意位置按住中键拖动地图；方向键与滚轮也可平移、缩放相机，`Home` 恢复全览视角
- `R` 重置选择，`Esc` 返回控制台

使用 Ninja、MinGW Makefiles 等单配置生成器时，Windows 可执行文件也可能位于 `build\map_shortest_path.exe`。

程序默认加载 `data/curved/` 的新版 1448×1086 校园图数据：86 个道路及独立入口节点、115 条可通行边、297 个道路控制点和 38 个地点。加载时会把字符串节点 ID 稳定映射为整数 ID，使用折线长度换算的 `distance_m_est_curved` 作为边权。地点中心仅用于显示，寻路从对应道路入口开始；运行时连同显示锚点共得到 124 个节点、153 条逻辑边和 373 个有效渲染点。

旧版 `map.txt` 和 CSV 仍可继续加载，也可以指定其他地图文件：

```bash
./build/map_shortest_path path/to/map.txt
```

---

## 使用说明

1. 启动程序。
2. 在控制台地图中查看节点、道路和权值。
3. 选择起点节点和终点节点。
4. 选择 Dijkstra 或 A* 算法。
5. 选择慢速、正常或快速动画。
6. 启动可视化，观察节点访问顺序和当前节点。
7. 搜索结束后查看高亮路径、总距离和访问节点数。
8. 使用重置功能清除选择并恢复默认速度。

---

## 可视化说明

系统中可以使用不同颜色表示不同状态：

| 类型    | 含义            |
| ----- | ------------- |
| 普通节点  | 尚未访问的节点       |
| 起点    | 路径搜索的开始位置     |
| 终点    | 路径搜索的目标位置     |
| 已访问节点 | 算法搜索过程中访问过的节点 |
| 当前节点  | 当前正在扩展的节点     |
| 最短路径  | 最终计算得到的最优路径   |

---

## 示例结果

例如，从节点 `A` 到节点 `F` 的最短路径可能为：

```text
A -> B -> C -> F
```

系统输出结果示例：

```text
Shortest Path: A -> B -> C -> F
Total Distance: 12
```

同时，控制台可视化界面会高亮显示这条路径。

---

## Dijkstra 与 A* 对比

| 对比项      | Dijkstra | A*          |
| -------- | -------- | ----------- |
| 是否使用启发函数 | 否        | 是           |
| 是否保证最短路径 | 是        | 是，前提是启发函数合理 |
| 搜索范围     | 通常较大     | 通常较小        |
| 适用场景     | 普通加权图    | 地图导航、游戏寻路   |
| 核心依据     | 当前最短距离   | 实际代价 + 估计代价 |

---

## 项目亮点

- 将抽象算法过程变得直观易懂
- 适合课堂展示和课程设计答辩
- 支持两种经典最短路径算法
- 使用控制台字符地图和 ANSI 颜色展示路径搜索过程
- 具有较好的扩展性，可以继续添加更多算法

---

## 后续改进方向

- 使用邻接表和最小堆优化路径搜索
- 支持更大规模地图
- 支持动态路况与边权修改
- 增加更多 POI 和地点分类
- 增加路线偏好设置
- 完善多平台 3D 构建
- 继续拆分 3D 渲染模块
- 增加更完善的性能测试

---

## 适用场景

- 数据结构课程设计
- 算法可视化项目
- 图论学习工具
- 人工智能搜索算法演示
- 地图导航系统原型
- 游戏路径规划原型

---

## 作者

| 成员  | 分工        |
| --- | --------- |
| 仵怡飞 | 代码开发与测试   |
| 李欣雨 | 项目设计与算法   |
| 李悦涵 | 项目文档      |
| 廖若男 | PPT 与答辩材料 |

GitHub: [@xinsuhan](https://github.com/xinsuhan)

---

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for the full text.

```text
MIT License

Copyright (c) 2026 xinsuhan

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files, to deal in the Software
without restriction, including without limitation the rights to use, copy,
modify, merge, publish, distribute, sublicense, and/or sell copies of the Software.
```
