#ifndef MAP_SHORTEST_PATH_PATH_RESULT_H
#define MAP_SHORTEST_PATH_PATH_RESULT_H

#include "models.h"

PathResult *path_result_create(void);
void path_result_destroy(PathResult *result);
void path_result_reset(PathResult *result);
int path_result_build(const Graph *graph, int goal_index,
                      const int previous[], PathResult *result);

#endif
