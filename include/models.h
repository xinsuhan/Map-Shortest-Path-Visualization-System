#ifndef MAP_SHORTEST_PATH_MODELS_H
#define MAP_SHORTEST_PATH_MODELS_H

#include "common.h"

typedef enum {
    NODE_LANDMARK = 0,
    NODE_JUNCTION,
    NODE_LAKE,
    NODE_SQUARE,
    NODE_GATE,
    NODE_BRIDGE,
    NODE_ENTRANCE,
    NODE_SERVICE
} NodeType;

typedef struct {
    int id;
    char name[MSP_NAME_LENGTH];
    double x;
    double y;
    NodeType type;
    float width;
    float depth;
    float height;
    int visible;
} Node;

typedef enum {
    ROAD_MAIN = 0,
    ROAD_BRANCH,
    ROAD_ENTRANCE,
    ROAD_BRIDGE
} RoadType;

typedef struct {
    double x;
    double y;
} MapPoint;

typedef struct {
    int from_id;
    int to_id;
    double weight;
    RoadType type;
    int walkable;
    int geometry_start;
    int geometry_count;
} Edge;

typedef struct {
    int id;
    char name[MSP_NAME_LENGTH];
    char alias[MSP_NAME_LENGTH];
    int display_node_id;
    int entrance_node_id;
    char category[MSP_CATEGORY_LENGTH];
} Place;

typedef struct {
    Place places[MSP_MAX_PLACES];
    int place_count;
} PlaceStore;

typedef struct {
    Node nodes[MSP_MAX_NODES];
    int node_count;
    Edge edges[MSP_MAX_EDGES];
    int edge_count;
    MapPoint geometry_points[MSP_MAX_GEOMETRY_POINTS];
    int geometry_point_count;
    int weights_in_meters;
    double adjacency[MSP_MAX_NODES][MSP_MAX_NODES];
} Graph;

typedef struct {
    int found;
    double total_distance;
    int path[MSP_MAX_NODES];
    int path_length;
    int visited_order[MSP_MAX_NODES];
    int visited_count;
} PathResult;

#endif
