#ifndef MAP_SHORTEST_PATH_MAP_TRANSFORM_H
#define MAP_SHORTEST_PATH_MAP_TRANSFORM_H

#include "raylib.h"

typedef struct {
    float scale_x;
    float scale_y;
    float offset_x;
    float offset_y;
    int flip_y;
} MapTransform;

Vector3 map_to_world(float map_x, float map_y, const MapTransform *transform);

#endif
