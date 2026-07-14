#include "ui_text.h"

#include <stddef.h>
#include <string.h>

typedef struct {
    const char *internal_name;
    const char *display_name;
} UiNameMapping;

static const char *const UI_TEXTS[] = {
    "四川大学江安校区导航",
    "四川大学江安校区",
    "校园导航",
    "起点",
    "在地图上单击选择起点",
    "终点",
    "在地图上右键选择终点",
    "路径算法",
    "开始导航",
    "请选择起点和终点",
    "请先选择起点和终点",
    "起点和终点不能相同",
    "正在规划推荐路线……",
    "未找到可用路线",
    "推荐路线已生成",
    "请点击开始导航",
    "路线已清除",
    "路径详情",
    "请选择两个地点以查看距离和步行时间。",
    "米",
    "距离单位",
    "路线节点",
    "搜索图书馆、食堂、体育馆……",
    "地图图例",
    "教学区",
    "宿舍 / 餐饮",
    "绿地",
    "水域",
    "道路",
    "路线",
    "附近未找到可通行道路"
};

static const UiNameMapping PLACE_NAMES[] = {
    {"Mingyuan Lake", "明远湖"},
    {"Arts College", "艺术学院"},
    {"Campus Stadium", "校园体育场"},
    {"West Dormitory", "西区宿舍"},
    {"East Dormitory", "东区宿舍"},
    {"Disaster Reconstruction and Management College", "灾后重建与管理学院"},
    {"Southwest Gate", "西南门"},
    {"Library", "图书馆"},
    {"Architecture and Environment College Building", "建筑与环境学院楼"},
    {"Second Basic Building", "二基楼"},
    {"East Canteen", "东园食堂"},
    {"West Canteen", "西园食堂"},
    {"Gymnasium", "体育馆"},
    {"Liberal Arts Buildings", "文科楼群"},
    {"Aerospace Building", "空天楼"},
    {"First Teaching Building", "第一教学楼"},
    {"Comprehensive and First Basic Buildings", "综合楼与一基楼"},
    {"Administration Building", "行政楼"},
    {"SouthwestGate", "西南门"},
    {"XiyuanDorm", "西园宿舍"},
    {"XiyuanCanteen", "西园食堂"},
    {"YouthSquare", "青春广场"},
    {"WestSportsField", "西区运动场"},
    {"LongBridge", "长桥"},
    {"JiangAnLibrary", "图书馆"},
    {"FirstTeachingBuilding", "一教楼"},
    {"ComprehensiveBuilding", "综合楼"},
    {"AdministrationBuilding", "行政楼"},
    {"FirstBasicBuilding", "一基楼"},
    {"JiangAnGym", "体育馆"},
    {"EastGate", "东门"},
    {"SouthGate", "南门"},
    {"MingyuanLake", "明远湖"},
    {"SwimmingPool", "游泳馆"},
    {"LiberalArtsBuildings", "文科楼群"},
    {"SecondBasicBuilding", "二基楼"},
    {"DongyuanDorm", "东园宿舍"},
    {"InterdisciplinaryArtCenter", "交叉学科艺术中心"},
    {"ArtsCollege", "艺术学院"},
    {"ArchitectureEnvironmentCollege", "建筑与环境学院"},
    {"DisasterManagementCollege", "灾后重建与管理学院"},
    {"EngineeringTrainingCenter", "工程训练中心"},
    {"DongyuanCanteen", "东园食堂"}
};

const char *ui_text(UiTextId id) {
    size_t count = sizeof(UI_TEXTS) / sizeof(UI_TEXTS[0]);
    return id >= 0 && (size_t)id < count ? UI_TEXTS[id] : "";
}

const char *ui_place_name(const char *internal_name) {
    size_t i;
    if (internal_name == NULL) return "";
    for (i = 0; i < sizeof(PLACE_NAMES) / sizeof(PLACE_NAMES[0]); ++i) {
        if (strcmp(internal_name, PLACE_NAMES[i].internal_name) == 0) {
            return PLACE_NAMES[i].display_name;
        }
    }
    return internal_name;
}

const char *ui_font_glyphs(void) {
    return "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"
           "DijkstraA*+-[]()./% "
           "四川大学江安校区导航校园起点终点在地图上单击选择右键路径算法开始"
           "请选择和先不能相同正在规划推荐路线未找到可用已生成点击清除详情"
           "两个地点以查看距离步行时间米单位节点搜索图书馆食堂体育馆图例"
           "教学宿舍餐饮绿地水域道路明远湖艺术学院西区东区西南门一基楼"
           "二基楼东园西园工程训练中心文理综合校医院求是广场青春运动场"
           "长桥一教楼行政楼游泳馆文科楼群交叉学科建筑与环境灾后重建管理空天与第一"
           "共约分钟个更多另有无结果放大缩小复位三维二维附近通。、……";
}
