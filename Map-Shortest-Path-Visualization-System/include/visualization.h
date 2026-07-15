#ifndef MAP_SHORTEST_PATH_VISUALIZATION_H
#define MAP_SHORTEST_PATH_VISUALIZATION_H

#include "models.h"

void visualization_print_graph(const Graph *graph);
void visualization_print_search(const Graph *graph, const PathResult *result);
void visualization_print_path(const Graph *graph, const PathResult *result);
void visualization_initialize_console(void);
void visualization_clear_screen(void);
void visualization_draw_map(const Graph *graph, int start_id, int goal_id,
                            const int visited_order[], int visited_count,
                            int current_id, const PathResult *final_result);
void visualization_play_search(const Graph *graph, const PathResult *result,
                               int start_id, int goal_id, int delay_ms);

#endif
