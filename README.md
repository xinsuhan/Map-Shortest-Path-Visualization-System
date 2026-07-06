# Map Shortest Path Visualization System

## 项目简介

**Map Shortest Path Visualization System** 是一个地图最短路径可视化系统，主要用于展示图结构中最短路径算法的运行过程。

本项目支持 Dijkstra 算法和 A* 算法。当前版本提供终端菜单，可选择起点、终点和算法，并通过控制台字符地图显示节点访问顺序、最终路径及总距离。

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
- 核心算法：Dijkstra Algorithm、A* Search Algorithm
- 核心数据结构：
  - Graph 图
  - Node 节点
  - Edge 边
  - Array 数组
  - Adjacency Matrix 邻接矩阵
- 数据存储：`map.txt`（默认）与 CSV
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

### 2. 编译项目

```bash
cmake -S . -B build
cmake --build build
```

### 3. 运行测试

```bash
ctest --test-dir build --output-on-failure
```

### 4. 运行程序

Linux / macOS：

```bash
./build/map_shortest_path
```

Windows：

```powershell
build\Debug\map_shortest_path.exe
```

对于单配置生成器，Windows 可执行文件也可能位于 `build\map_shortest_path.exe`。

程序默认加载 `data/map.txt`，其中包含四川大学江安校区西区的 16 个简化地标和 32 条双向道路。也可以指定其他地图文件：

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

- 增加 BFS、DFS、Floyd、Bellman-Ford 等算法
- 支持用户手动添加节点和边
- 支持动态修改边的权重
- 支持导入真实地图数据
- 支持自定义算法动画速度
- 增加暂停和继续功能
- 优化路径搜索动画效果
- 优化界面交互效果
- 支持保存和加载地图文件

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
