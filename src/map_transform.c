#include "map_transform.h"

Vector3 map_to_world(float map_x, float map_y, const MapTransform *transform) {
    float mapped_y = map_y;
    if (transform == 0) return (Vector3){map_x, 0.0f, map_y};
    if (transform->flip_y) mapped_y = -map_y;
    return (Vector3){map_x * transform->scale_x + transform->offset_x,
                     0.0f,
                     mapped_y * transform->scale_y + transform->offset_y};
}
