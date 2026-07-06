#ifndef MAP_SHORTEST_PATH_MODELS_H
#define MAP_SHORTEST_PATH_MODELS_H

#include "common.h"

typedef struct {
    int id;
    char name[MSP_NAME_LENGTH];
    double x;
    double y;
} Node;

typedef struct {
    int from_id;
    int to_id;
    double weight;
} Edge;

typedef struct {
    Node nodes[MSP_MAX_NODES];
    int node_count;
    Edge edges[MSP_MAX_EDGES];
    int edge_count;
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

