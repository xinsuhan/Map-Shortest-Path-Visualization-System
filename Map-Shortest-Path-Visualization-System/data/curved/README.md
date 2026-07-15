# 四川大学江安校区曲线路网增强版数据

本数据基于用户提供的 1448×1086 新版手绘校园图 `campus_map_new.png` 重建。
道路折线是渲染与寻路共用的唯一数据源，不再复用旧图坐标。

## 文件
- `campus_graph_curved.json`：推荐直接给程序使用。每条边新增 `geometry` 折线点数组。
- `edges_curved.csv`：边摘要与曲线路径长度。
- `edge_geometry_points.csv`：每条边的所有折线控制点，适合 CSV 工作流。
- `nodes.csv`：道路节点。
- `pois.csv`：兴趣点。
- `curved_graph_overlay_preview.png`：曲线路网覆盖预览。
- `rebuild_map_data.py`：从已人工标定的道路/入口生成全部发布数据。

## 前端绘制方式
算法搜索仍按 `source` / `target` 计算最短路；渲染一条边时不要直接画 source 到 target 的直线，而要依次连接该边 `geometry` 中的所有点。

## 重要说明
这是课程项目级的近似模拟数据，不是四川大学官方 GIS 数据，也不是测绘级道路数据。
当前包含 86 个道路及独立入口节点、115 条双向可通行道路、297 个道路几何点和
38 个图中命名地点。普通建筑坐标只用于标签显示，算法从其独立道路入口开始寻路。
