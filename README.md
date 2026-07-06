# Map Shortest Path Visualization System

## 项目简介

**Map Shortest Path Visualization System** 是一个地图最短路径可视化系统，主要用于展示图结构中最短路径算法的运行过程。

本项目支持 **Dijkstra 算法** 和 **A* 算法**。用户可以在图形界面中选择起点和终点，系统会自动计算两点之间的最短路径，并通过可视化方式展示搜索过程和最终路径结果。

该项目适合作为数据结构、算法设计、图论、人工智能搜索算法或课程设计项目。

---

## 项目功能

- 地图 / 图结构可视化展示
- 支持节点和边的图形化显示
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

传统最短路径算法通常只输出结果，而本项目通过图形界面展示搜索过程，使算法执行过程更加直观。

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

相比 Dijkstra 算法，A* 算法在地图场景中通常搜索范围更小，效率更高。

---

## 技术栈

> 可根据实际代码情况修改。

- 编程语言：C++ / Python / JavaScript
- 核心算法：Dijkstra Algorithm、A* Search Algorithm
- 核心数据结构：
  - Graph 图
  - Node 节点
  - Edge 边
  - Priority Queue 优先队列
  - Adjacency List 邻接表
- 图形展示：可视化界面 / 图形窗口 / 地图展示模块

---

## 项目结构

```text
Map-Shortest-Path-Visualization-System/
│
├── src/                     # 源代码目录
│   ├── main.cpp             # 程序入口
│   ├── graph.cpp            # 图结构实现
│   ├── dijkstra.cpp         # Dijkstra 算法实现
│   ├── astar.cpp            # A* 算法实现
│   └── visualization.cpp    # 可视化展示模块
│
├── include/                 # 头文件目录
│   ├── graph.h
│   ├── dijkstra.h
│   ├── astar.h
│   └── visualization.h
│
├── data/                    # 地图数据或测试数据
├── assets/                  # 图片、图标等资源文件
├── screenshots/             # 项目运行截图
├── README.md                # 项目说明文档
└── LICENSE                  # 开源协议
```

如果实际项目结构不同，可以根据真实文件名修改这一部分。

---

## 安装与运行

### 1. 克隆仓库

```bash
git clone https://github.com/xinsuhan/Map-Shortest-Path-Visualization-System.git
cd Map-Shortest-Path-Visualization-System
```

### 2. 编译项目

如果项目使用 C++，可以使用：

```bash
g++ src/*.cpp -o shortest_path
```

如果使用 CMake，可以使用：

```bash
mkdir build
cd build
cmake ..
make
```

### 3. 运行程序

Linux / macOS：

```bash
./shortest_path
```

Windows：

```bash
shortest_path.exe
```

---

## 使用说明

1. 启动程序。
2. 在界面中查看地图或图结构。
3. 选择起点节点。
4. 选择终点节点。
5. 选择要使用的算法：Dijkstra 或 A*。
6. 点击开始按钮运行算法。
7. 系统会显示搜索过程，并高亮最终最短路径。
8. 查看最短路径经过的节点以及路径总长度。

---

## 可视化说明

系统中可以使用不同颜色表示不同状态：

| 类型 | 含义 |
| --- | --- |
| 普通节点 | 尚未访问的节点 |
| 起点 | 路径搜索的开始位置 |
| 终点 | 路径搜索的目标位置 |
| 已访问节点 | 算法搜索过程中访问过的节点 |
| 当前节点 | 当前正在扩展的节点 |
| 最短路径 | 最终计算得到的最优路径 |

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

同时，图形界面会高亮显示这条路径。

---

## Dijkstra 与 A* 对比

| 对比项 | Dijkstra | A* |
| --- | --- | --- |
| 是否使用启发函数 | 否 | 是 |
| 是否保证最短路径 | 是 | 是，前提是启发函数合理 |
| 搜索范围 | 通常较大 | 通常较小 |
| 适用场景 | 普通加权图 | 地图导航、游戏寻路 |
| 核心依据 | 当前最短距离 | 实际代价 + 估计代价 |

---

## 项目亮点

- 将抽象算法过程变得直观易懂
- 适合课堂展示和课程设计答辩
- 支持两种经典最短路径算法
- 使用图形化方式展示路径搜索过程
- 具有较好的扩展性，可以继续添加更多算法

---

## 后续改进方向

- 增加 BFS、DFS、Floyd、Bellman-Ford 等算法
- 支持用户手动添加节点和边
- 支持动态修改边的权重
- 支持导入真实地图数据
- 增加算法执行速度控制
- 增加暂停、继续、重置功能
- 增加路径搜索过程动画
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

**su xinxin**

GitHub: [@xinsuhan](https://github.com/xinsuhan)

---

## License

This project is licensed under the MIT License.

```text
MIT License

Copyright (c) 2026 su xinxin

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files, to deal in the Software
without restriction, including without limitation the rights to use, copy,
modify, merge, publish, distribute, sublicense, and/or sell copies of the Software.
```
