#!/usr/bin/env python3
"""Rebuild the Jiang'an routing graph for campus_map_new.png.

The road polylines below are the single source of truth for both routing and
rendering.  POI coordinates are display anchors only; routing starts at the
named road entrance in the final column of pois.csv.
"""

from __future__ import annotations

import csv
import json
import math
from pathlib import Path


HERE = Path(__file__).resolve().parent
MAP_WIDTH = 1448
MAP_HEIGHT = 1086
METERS_PER_PIXEL = 0.8


NODE_ROWS = [
    # West gate and the west dormitory road grid.
    ("J01", 195, 900, "西南门入口"), ("J02", 205, 850, "西园南路西口"),
    ("J03", 205, 790, "西园四路西口"), ("J04", 205, 730, "西园三路西口"),
    ("J05", 205, 675, "西园二路西口"), ("J06", 205, 620, "西园一路西口"),
    ("J07", 205, 575, "西园北路西口"), ("J08", 165, 550, "长城路中段"),
    ("J09", 160, 470, "长城路北段"), ("J10", 190, 420, "西北环路口"),
    ("J11", 245, 420, "西北环路中段"), ("J12", 315, 430, "西北环路东段"),
    ("J13", 390, 435, "安宁河西北口"), ("J14", 450, 440, "明远大道西口"),
    ("J32", 285, 850, "西园南路中口"), ("J33", 400, 825, "西园南路东口"),
    ("J34", 285, 790, "西园四路中口"), ("J35", 405, 770, "西园四路东口"),
    ("J36", 470, 765, "西园东南口"), ("J37", 285, 730, "西园三路中口"),
    ("J38", 405, 715, "西园三路东口"), ("J39", 470, 700, "西园东三口"),
    ("J40", 285, 675, "西园二路中口"), ("J41", 410, 675, "西园二路东口"),
    ("J42", 450, 680, "西园东二口"), ("J43", 285, 620, "西园一路中口"),
    ("J44", 410, 620, "西园一路东口"), ("J45", 450, 630, "长桥西端"),
    ("J46", 285, 575, "西园北路中口"), ("J47", 405, 575, "西园北路东口"),
    ("J48", 450, 580, "学生服务中心路口"),

    # Main avenue north of Mingyuan Lake and the east/south perimeter.
    ("J15", 473, 375, "明远大道西弯"), ("J16", 540, 335, "明远大道西段"),
    ("J17", 630, 320, "明远大道中西段"), ("J18", 735, 302, "明远大道中段"),
    ("J19", 842, 297, "行政楼路口"), ("J20", 910, 360, "中央主路北口"),
    ("J21", 980, 420, "中央主路中口"), ("J22", 1045, 480, "中央主路南口"),
    ("J23", 1105, 540, "体育区西南口"), ("J24", 1185, 580, "东南门路口"),
    ("J25", 1230, 655, "东实北路口"), ("J26", 1250, 735, "东实南路口"),
    ("J27", 450, 850, "江安大道南段西"), ("J28", 610, 815, "江安大道南段中西"),
    ("J29", 760, 775, "南门西路口"), ("J30", 900, 735, "南门东路口"),
    ("J31", 1035, 685, "江安大道南段东"),

    # Two central academic corridors and the two real bridges.
    ("J49", 610, 650, "长桥东端"), ("J50", 690, 635, "实践楼西口"),
    ("J51", 785, 600, "学术交流中心南口"), ("J52", 820, 535, "中央广场西口"),
    ("J53", 875, 500, "一教西北口"), ("J54", 970, 445, "文科区西口"),
    ("J55", 1060, 390, "体育馆西北口"), ("J56", 1140, 320, "游泳馆北口"),
    ("J57", 650, 710, "南桥西端"), ("J58", 780, 670, "南桥东端"),
    ("J59", 880, 630, "礼仪堂西口"), ("J60", 980, 585, "二教西口"),
    ("J61", 1085, 530, "博物馆西口"),

    # East dormitory and staff-housing street grid.
    ("J62", 785, 85, "东园北入口"), ("J63", 790, 150, "东园西纵路北"),
    ("J64", 820, 220, "东园西纵路中"), ("J65", 845, 280, "东园西纵路南"),
    ("J66", 900, 205, "东园中路口"), ("J67", 955, 185, "东园东路口"),
    ("J68", 1010, 260, "文科楼北口"), ("J69", 1100, 275, "学术文化中心路口"),
    ("J70", 1195, 230, "东校门"),

    # Northwest research area.
    ("J71", 75, 155, "西北门内口"), ("J72", 185, 150, "交叉学科西路口"),
    ("J73", 285, 155, "交叉学科中路口"), ("J74", 390, 170, "交叉学科东路口"),
    ("J75", 390, 260, "未来研究院南口"), ("J76", 250, 260, "交叉研究楼南口"),
    ("J77", 140, 290, "新能源研究院路口"),

    # Separate entrances for nearby POIs which share the same main-road junction.
    ("P01", 805, 520, "明远湖东入口"), ("P02", 895, 520, "综合楼入口"),
    ("P03", 805, 135, "教职工楼入口"), ("P04", 910, 220, "东园一二舍入口"),
    ("P05", 925, 210, "东园食堂入口"), ("P06", 1165, 245, "留学生公寓入口"),
    ("P07", 1240, 245, "灾后重建学院入口"), ("P08", 1280, 720, "东实食堂入口"),
    ("P09", 270, 175, "重点实验基地入口"),
]


EDGES: list[tuple[str, str, str, list[tuple[float, float]] | None, str, str]] = []


def edge(a, b, name, points=None, kind="road", width="normal"):
    EDGES.append((a, b, name, points, kind, width))


# West boundary and dormitory grid.
for a, b in [("J01", "J02"), ("J02", "J03"), ("J03", "J04"),
             ("J04", "J05"), ("J05", "J06"), ("J06", "J07")]:
    edge(a, b, "西园西侧路", width="wide")
edge("J07", "J08", "西园西侧路", [(205, 575), (180, 563), (165, 550)], width="wide")
edge("J08", "J09", "长城路", [(165, 550), (158, 510), (160, 470)], width="wide")
edge("J09", "J10", "西北环路", [(160, 470), (170, 442), (190, 420)], width="wide")
for a, b in [("J10", "J11"), ("J11", "J12"), ("J12", "J13"), ("J13", "J14")]:
    edge(a, b, "西北环路", width="wide")

for row in [
    ("J02", "J32", "J33", "J27"), ("J03", "J34", "J35", "J36"),
    ("J04", "J37", "J38", "J39"), ("J05", "J40", "J41", "J42"),
    ("J06", "J43", "J44", "J45"), ("J07", "J46", "J47", "J48"),
]:
    for a, b in zip(row, row[1:]):
        edge(a, b, "西园宿舍横向路")
for column in [
    ("J32", "J34", "J37", "J40", "J43", "J46"),
    ("J33", "J35", "J38", "J41", "J44", "J47"),
    ("J27", "J36", "J39", "J42", "J45", "J48"),
]:
    for a, b in zip(column, column[1:]):
        edge(a, b, "西园宿舍纵向路")

# South perimeter road.
edge("J02", "J27", "江安大道南段", [(205, 850), (320, 845), (450, 850)], width="wide")
edge("J27", "J28", "江安大道南段", [(450, 850), (525, 832), (610, 815)], width="wide")
edge("J28", "J29", "江安大道南段", [(610, 815), (690, 795), (760, 775)], width="wide")
edge("J29", "J30", "江安大道南段", [(760, 775), (830, 755), (900, 735)], width="wide")
edge("J30", "J31", "江安大道南段", [(900, 735), (970, 710), (1035, 685)], width="wide")
edge("J31", "J24", "江安大道东段", [(1035, 685), (1110, 635), (1185, 580)], width="wide")
edge("J24", "J25", "东实连接路", [(1185, 580), (1215, 610), (1230, 655)], width="wide")
edge("J25", "J26", "东实连接路", [(1230, 655), (1240, 695), (1250, 735)], width="wide")

# Mingyuan Avenue and the central diagonal main road.
edge("J14", "J15", "明远大道", [(450, 440), (455, 410), (473, 375)], width="wide")
edge("J15", "J16", "明远大道", [(473, 375), (500, 350), (540, 335)], width="wide")
edge("J16", "J17", "明远大道", [(540, 335), (585, 325), (630, 320)], width="wide")
edge("J17", "J18", "明远大道", [(630, 320), (680, 310), (735, 302)], width="wide")
edge("J18", "J19", "明远大道", [(735, 302), (790, 295), (842, 297)], width="wide")
edge("J19", "J20", "中央教学区主路", [(842, 297), (870, 325), (910, 360)], width="wide")
edge("J20", "J21", "中央教学区主路", [(910, 360), (945, 390), (980, 420)], width="wide")
edge("J21", "J22", "中央教学区主路", [(980, 420), (1010, 450), (1045, 480)], width="wide")
edge("J22", "J23", "中央教学区主路", [(1045, 480), (1075, 510), (1105, 540)], width="wide")
edge("J23", "J24", "中央教学区主路", [(1105, 540), (1145, 560), (1185, 580)], width="wide")

# Central corridors.  Only the two marked bridge segments may cross water.
edge("J45", "J49", "明远湖长桥", [(450, 630), (525, 638), (610, 650)], kind="bridge", width="wide")
edge("J49", "J50", "实践楼北路", [(610, 650), (650, 645), (690, 635)])
edge("J50", "J51", "实践楼北路", [(690, 635), (735, 620), (785, 600)])
edge("J51", "J52", "中央广场西路", [(785, 600), (802, 565), (820, 535)])
edge("J52", "J53", "教学区中路", [(820, 535), (845, 515), (875, 500)])
edge("J53", "J54", "教学区中路", [(875, 500), (920, 475), (970, 445)])
edge("J54", "J55", "教学区中路", [(970, 445), (1015, 415), (1060, 390)])
edge("J55", "J56", "体育馆北路", [(1060, 390), (1100, 355), (1140, 320)])
edge("J28", "J57", "南桥西接入", [(610, 815), (615, 775), (630, 735), (650, 710)])
edge("J57", "J58", "南桥", [(650, 710), (705, 695), (780, 670)], kind="bridge", width="wide")
edge("J58", "J59", "教学区南路", [(780, 670), (830, 650), (880, 630)])
edge("J59", "J60", "教学区南路", [(880, 630), (930, 607), (980, 585)])
edge("J60", "J61", "教学区南路", [(980, 585), (1030, 555), (1085, 530)])
edge("J61", "J23", "体育区南接入", [(1085, 530), (1095, 535), (1105, 540)])
edge("J51", "J58", "中央广场纵路", [(785, 600), (790, 635), (780, 670)])
edge("J55", "J61", "体育区西路", [(1060, 390), (1070, 455), (1085, 530)])
edge("J58", "J29", "南门西接入", [(780, 670), (770, 720), (760, 775)])
edge("J59", "J30", "南门中央接入", [(880, 630), (890, 680), (900, 735)])
edge("J60", "J31", "南门东接入", [(980, 585), (1005, 635), (1035, 685)])
edge("J21", "J54", "文科区接入路")
# Direct eastbound branch at the central-road junction.  Without this edge,
# routes to the gym are forced south to J54 and draw a false V-shaped detour.
edge("J21", "J55", "体育馆北接入路",
     [(980, 420), (1018, 405), (1060, 390)])
edge("J22", "J61", "体育区接入路")

# East dormitory grid and east gate.
edge("J62", "J63", "东园西纵路", [(785, 85), (780, 115), (790, 150)])
edge("J63", "J64", "东园西纵路", [(790, 150), (800, 185), (820, 220)])
edge("J64", "J65", "东园西纵路", [(820, 220), (833, 250), (845, 280)])
edge("J65", "J19", "东园南接入", [(845, 280), (842, 297)])
edge("J63", "J66", "东园北路", [(790, 150), (845, 175), (900, 205)])
edge("J66", "J67", "东园北路", [(900, 205), (930, 195), (955, 185)])
edge("J67", "J68", "东园东纵路", [(955, 185), (980, 220), (1010, 260)])
edge("J68", "J69", "东园南路", [(1010, 260), (1055, 268), (1100, 275)])
edge("J69", "J70", "东校门路", [(1100, 275), (1145, 250), (1195, 230)], width="wide")
edge("J65", "J68", "东园中路", [(845, 280), (925, 270), (1010, 260)])
edge("J68", "J20", "文科楼西接入", [(1010, 260), (965, 305), (910, 360)])
edge("J69", "J56", "体育区北接入", [(1100, 275), (1120, 297), (1140, 320)])
edge("J70", "J24", "东侧外环路", [(1195, 230), (1280, 285), (1360, 350),
                                      (1400, 430), (1360, 500), (1280, 545),
                                      (1185, 580)], width="wide")

# Northwest research area, connected to the west and lake roads.
edge("J74", "J75", "未来研究院东路", [(390, 170), (392, 215), (390, 260)])
edge("J75", "J14", "安宁河北岸路", [(390, 260), (420, 300), (445, 350), (450, 440)])
edge("J72", "J76", "交叉学科西路", [(185, 150), (210, 205), (250, 260)])
edge("J73", "J76", "交叉学科中路", [(285, 155), (290, 205), (250, 260)])
edge("J76", "J75", "交叉学科南路", [(250, 260), (320, 265), (390, 260)])
edge("J76", "J77", "新能源研究路", [(250, 260), (190, 275), (140, 290)])
edge("J71", "J77", "西北区外环路", [(75, 155), (70, 220), (90, 280), (140, 290)], width="wide")
edge("J77", "J09", "西北区南接入", [(140, 290), (145, 380), (160, 470)])

# Short, road-aligned POI entrance branches.  These prevent two distinct
# destinations from collapsing to the same routing node.
edge("J52", "P01", "明远湖东入口路")
edge("J53", "P02", "综合楼入口路")
edge("J63", "P03", "教职工楼入口路")
edge("J66", "P04", "东园一二舍入口路")
edge("J66", "P05", "东园食堂入口路")
edge("J70", "P06", "留学生公寓入口路")
edge("J70", "P07", "灾后重建学院入口路")
edge("J26", "P08", "东实食堂入口路")
edge("J73", "P09", "重点实验基地入口路")


POIS = [
    ("明远湖", 625, 450, "lake", "P01"),
    ("行政楼", 800, 335, "service", "J19"),
    ("图书馆", 735, 385, "teaching", "J18"),
    ("学术交流中心", 735, 535, "service", "J52"),
    ("第一教学楼", 935, 515, "teaching", "J53"),
    ("第二教学楼", 1015, 585, "teaching", "J60"),
    ("实践楼", 735, 635, "teaching", "J50"),
    ("礼仪堂", 925, 660, "service", "J59"),
    ("南门", 870, 760, "gate", "J30"),
    ("体育馆", 1100, 425, "sports", "J55"),
    ("体育运动场", 1230, 450, "sports", "J23"),
    ("游泳馆", 1145, 365, "sports", "J56"),
    ("江安校区图书馆与博物馆", 1165, 525, "teaching", "J24"),
    ("文科楼", 1020, 315, "teaching", "J68"),
    ("学术文化交流中心", 1180, 320, "service", "J69"),
    ("东园一舍与二舍", 915, 255, "dormitory", "P04"),
    ("东园六舍", 830, 165, "dormitory", "J63"),
    ("东园七舍", 900, 185, "dormitory", "J66"),
    ("东园八舍", 980, 165, "dormitory", "J67"),
    ("东园留学生公寓与教师公寓", 1100, 190, "dormitory", "P06"),
    ("教职工楼", 835, 105, "dormitory", "P03"),
    ("灾后重建与管理学院", 1300, 245, "teaching", "P07"),
    ("建筑与环境学院楼", 1320, 245, "teaching", "J70"),
    ("工程训练中心", 1320, 620, "teaching", "J25"),
    ("创新创业产业园", 1340, 700, "service", "J26"),
    ("东实学生食堂", 1370, 745, "canteen", "P08"),
    ("西园宿舍区", 250, 730, "dormitory", "J37"),
    ("西园食堂", 380, 660, "canteen", "J41"),
    ("学生服务中心", 470, 550, "service", "J48"),
    ("西南门", 200, 905, "gate", "J01"),
    ("西区体育场", 550, 740, "sports", "J57"),
    ("多学科交叉楼", 320, 150, "teaching", "J73"),
    ("国家重点工程研究实验基地", 260, 190, "teaching", "P09"),
    ("计算理论与未来研究院", 375, 195, "teaching", "J74"),
    ("多学科交叉研究机构大楼", 225, 235, "teaching", "J76"),
    ("江安新能源研究院", 220, 300, "teaching", "J77"),
    ("东园食堂", 935, 225, "canteen", "P05"),
    ("综合楼与一基楼", 950, 535, "teaching", "P02"),
]


def distance(points):
    return sum(math.hypot(b[0] - a[0], b[1] - a[1]) for a, b in zip(points, points[1:]))


def write_csv(path, fieldnames, rows):
    with path.open("w", encoding="utf-8", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)


def main():
    node_by_id = {node_id: (x, y) for node_id, x, y, _ in NODE_ROWS}
    node_rows = [
        {"id": node_id, "x": x, "y": y, "nx": round(x / MAP_WIDTH, 6),
         "ny": round(y / MAP_HEIGHT, 6), "name": name, "kind": "intersection"}
        for node_id, x, y, name in NODE_ROWS
    ]
    edge_rows = []
    geometry_rows = []
    json_edges = []
    for index, (source, target, road, points, kind, width) in enumerate(EDGES, 1):
        edge_id = f"E{index:03d}"
        if points is None:
            points = [node_by_id[source], node_by_id[target]]
        points = list(points)
        if tuple(points[0]) != node_by_id[source] or tuple(points[-1]) != node_by_id[target]:
            raise ValueError(f"{edge_id} geometry endpoints do not match {source}->{target}")
        pixel_length = distance(points)
        direct_length = math.dist(node_by_id[source], node_by_id[target])
        metric_length = pixel_length * METERS_PER_PIXEL
        edge_rows.append({
            "id": edge_id, "source": source, "target": target, "road": road,
            "kind": kind, "oneway": "False", "width": width,
            "distance_px": round(direct_length, 6),
            "distance_m_est": round(direct_length * METERS_PER_PIXEL, 6),
            "distance_px_curved": round(pixel_length, 6),
            "distance_m_est_curved": round(metric_length, 6),
            "geometry_point_count": len(points), "walkable": 1,
        })
        for order, (x, y) in enumerate(points):
            geometry_rows.append({"edge_id": edge_id, "source": source,
                                  "target": target, "road": road,
                                  "point_order": order, "x": x, "y": y})
        json_edges.append({"id": edge_id, "source": source, "target": target,
                           "road": road, "kind": kind, "oneway": False,
                           "width": width, "walkable": True,
                           "distance_m_est_curved": round(metric_length, 6),
                           "geometry": points})

    poi_rows = [
        {"id": f"POI{index:02d}", "name": name, "x": x, "y": y,
         "category": category, "nearest_node": entrance}
        for index, (name, x, y, category, entrance) in enumerate(POIS, 1)
    ]
    write_csv(HERE / "nodes.csv", ["id", "x", "y", "nx", "ny", "name", "kind"], node_rows)
    write_csv(HERE / "edges_curved.csv",
              ["id", "source", "target", "road", "kind", "oneway", "width",
               "distance_px", "distance_m_est", "distance_px_curved",
               "distance_m_est_curved", "geometry_point_count", "walkable"], edge_rows)
    write_csv(HERE / "edge_geometry_points.csv",
              ["edge_id", "source", "target", "road", "point_order", "x", "y"],
              geometry_rows)
    write_csv(HERE / "pois.csv",
              ["id", "name", "x", "y", "category", "nearest_node"], poi_rows)

    document = {
        "metadata": {"source_image": "campus_map_new.png", "image_width": MAP_WIDTH,
                     "image_height": MAP_HEIGHT, "meters_per_pixel": METERS_PER_PIXEL,
                     "routing_geometry_is_render_geometry": True},
        "nodes": node_rows, "edges": json_edges, "pois": poi_rows,
    }
    (HERE / "campus_graph_curved.json").write_text(
        json.dumps(document, ensure_ascii=False, indent=2), encoding="utf-8")

    try:
        from PIL import Image, ImageDraw
        image = Image.open(HERE / "campus_map_new.png").convert("RGB")
        draw = ImageDraw.Draw(image, "RGBA")
        for edge_row, (_, _, _, points, _, width) in zip(edge_rows, EDGES):
            if points is None:
                points = [node_by_id[edge_row["source"]], node_by_id[edge_row["target"]]]
            draw.line(points, fill=(239, 78, 35, 210), width=5 if width == "wide" else 3,
                      joint="curve")
        for x, y in node_by_id.values():
            draw.ellipse((x - 3, y - 3, x + 3, y + 3), fill=(20, 72, 170, 240))
        image.save(HERE / "curved_graph_overlay_preview.png")
    except ImportError:
        print("Pillow not installed; skipped overlay preview")

    print(f"wrote {len(node_rows)} road nodes, {len(edge_rows)} roads, "
          f"{len(geometry_rows)} geometry points and {len(poi_rows)} POIs")


if __name__ == "__main__":
    main()
