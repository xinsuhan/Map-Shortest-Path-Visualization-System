# 四川大学江安校区曲线路网增强版数据

本数据基于用户提供的 1920×1356 官方校园地图图片和上一版拓扑节点/边进行增强。

## 文件
- `campus_graph_curved.json`：推荐直接给程序使用。每条边新增 `geometry` 折线点数组。
- `edges_curved.csv`：边摘要与曲线路径长度。
- `edge_geometry_points.csv`：每条边的所有折线控制点，适合 CSV 工作流。
- `nodes.csv`：道路节点。
- `pois.csv`：兴趣点。
- `curved_graph_overlay_preview.png`：曲线路网覆盖预览。

## 前端绘制方式
算法搜索仍按 `source` / `target` 计算最短路；渲染一条边时不要直接画 source 到 target 的直线，而要依次连接该边 `geometry` 中的所有点。

## 重要说明
这是课程项目级的近似模拟数据，不是四川大学官方 GIS 数据，也不是测绘级道路数据。地图中被文字、建筑轮廓或图标遮挡的道路仍可能需要人工微调。
