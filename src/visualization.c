#include "visualization.h"

#include "graph.h"
#include "storage.h"

#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#else
#include <time.h>
#endif

#define CANVAS_WIDTH 78
#define CANVAS_HEIGHT 26
#define MAP_X_SCALE 5
#define MAP_Y_SCALE 2
#define MAP_X_OFFSET 2
#define MAP_Y_OFFSET 1

#define COLOR_RESET "\033[0m"
#define COLOR_ROAD "\033[90m"
#define COLOR_START "\033[92m"
#define COLOR_GOAL "\033[91m"
#define COLOR_VISITED "\033[94m"
#define COLOR_CURRENT "\033[93m"
#define COLOR_PATH "\033[96m"
#define COLOR_NODE "\033[97m"

typedef struct {
    char cells[CANVAS_HEIGHT][CANVAS_WIDTH];
    unsigned char path_cells[CANVAS_HEIGHT][CANVAS_WIDTH];
    int node_ids[CANVAS_HEIGHT][CANVAS_WIDTH];
} ConsoleCanvas;

static int canvas_x(double logical_x) {
    return MAP_X_OFFSET + (int)((logical_x - 2.0) * MAP_X_SCALE + 0.5);
}

static int canvas_y(double logical_y) {
    return MAP_Y_OFFSET + (int)((12.0 - logical_y) * MAP_Y_SCALE + 0.5);
}

static int in_canvas(int x, int y) {
    return x >= 0 && x < CANVAS_WIDTH && y >= 0 && y < CANVAS_HEIGHT;
}

static void canvas_init(ConsoleCanvas *canvas) {
    int x;
    int y;
    for (y = 0; y < CANVAS_HEIGHT; ++y) {
        for (x = 0; x < CANVAS_WIDTH; ++x) {
            canvas->cells[y][x] = ' ';
            canvas->path_cells[y][x] = 0;
            canvas->node_ids[y][x] = -1;
        }
    }
}

static int result_contains_edge(const PathResult *result, int from_id, int to_id) {
    int i;
    if (result == NULL || !result->found) {
        return 0;
    }
    for (i = 0; i + 1 < result->path_length; ++i) {
        if ((result->path[i] == from_id && result->path[i + 1] == to_id) ||
            (result->path[i] == to_id && result->path[i + 1] == from_id)) {
            return 1;
        }
    }
    return 0;
}

static int result_contains_node(const PathResult *result, int node_id) {
    int i;
    if (result == NULL || !result->found) {
        return 0;
    }
    for (i = 0; i < result->path_length; ++i) {
        if (result->path[i] == node_id) {
            return 1;
        }
    }
    return 0;
}

static int visited_contains(const int visited_order[], int visited_count, int node_id) {
    int i;
    if (visited_order == NULL) {
        return 0;
    }
    for (i = 0; i < visited_count; ++i) {
        if (visited_order[i] == node_id) {
            return 1;
        }
    }
    return 0;
}

static void canvas_draw_line(ConsoleCanvas *canvas, int x0, int y0, int x1, int y1,
                             char symbol, int is_path) {
    int dx = x1 > x0 ? x1 - x0 : x0 - x1;
    int sx = x0 < x1 ? 1 : -1;
    int dy = y1 > y0 ? y0 - y1 : y1 - y0;
    int sy = y0 < y1 ? 1 : -1;
    int error = dx + dy;

    for (;;) {
        if (in_canvas(x0, y0)) {
            canvas->cells[y0][x0] = symbol;
            canvas->path_cells[y0][x0] = (unsigned char)is_path;
        }
        if (x0 == x1 && y0 == y1) {
            break;
        }
        {
            int doubled_error = 2 * error;
            if (doubled_error >= dy) {
                error += dy;
                x0 += sx;
            }
            if (doubled_error <= dx) {
                error += dx;
                y0 += sy;
            }
        }
    }
}

static void canvas_write_text(ConsoleCanvas *canvas, int x, int y, const char *text) {
    int i;
    for (i = 0; text[i] != '\0' && in_canvas(x + i, y); ++i) {
        canvas->cells[y][x + i] = text[i];
    }
}

static void canvas_draw_edge(ConsoleCanvas *canvas, const Graph *graph, const Edge *edge,
                             const PathResult *final_result) {
    const Node *from = graph_get_node(graph, edge->from_id);
    const Node *to = graph_get_node(graph, edge->to_id);
    int is_path;
    int x0;
    int y0;
    int x1;
    int y1;
    char weight[12];

    if (!edge->walkable || from == NULL || to == NULL) {
        return;
    }
    x0 = canvas_x(from->x);
    y0 = canvas_y(from->y);
    x1 = canvas_x(to->x);
    y1 = canvas_y(to->y);
    is_path = result_contains_edge(final_result, edge->from_id, edge->to_id);
    canvas_draw_line(canvas, x0, y0, x1, y1, is_path ? '#' : '.', is_path);

    snprintf(weight, sizeof(weight), "%.1f", edge->weight);
    canvas_write_text(canvas, (x0 + x1) / 2 - 1, (y0 + y1) / 2, weight);
}

static void canvas_draw_node(ConsoleCanvas *canvas, const Node *node, int start_id, int goal_id,
                             const int visited_order[], int visited_count,
                             int current_id, const PathResult *final_result) {
    int x = canvas_x(node->x);
    int y = canvas_y(node->y);
    char label[16];
    int i;
    int show_node = node->visible || node->id == start_id || node->id == goal_id ||
                    node->id == current_id ||
                    result_contains_node(final_result, node->id) ||
                    visited_contains(visited_order, visited_count, node->id);

    if (!show_node) {
        return;
    }

    if (node->type == NODE_JUNCTION) {
        if (in_canvas(x, y)) {
            canvas->cells[y][x] = '+';
            canvas->node_ids[y][x] = node->id;
        }
        return;
    }
    snprintf(label, sizeof(label), "[%02d]", node->id);
    for (i = 0; label[i] != '\0' && in_canvas(x + i - 1, y); ++i) {
        canvas->cells[y][x + i - 1] = label[i];
        canvas->node_ids[y][x + i - 1] = node->id;
    }
}

static const char *node_color(int node_id, int start_id, int goal_id,
                              const int visited_order[], int visited_count,
                              int current_id, const PathResult *final_result) {
    if (node_id == start_id) {
        return COLOR_START;
    }
    if (node_id == goal_id) {
        return COLOR_GOAL;
    }
    if (node_id == current_id) {
        return COLOR_CURRENT;
    }
    if (result_contains_node(final_result, node_id)) {
        return COLOR_PATH;
    }
    if (visited_contains(visited_order, visited_count, node_id)) {
        return COLOR_VISITED;
    }
    return COLOR_NODE;
}

static void sleep_ms(int delay_ms) {
    if (delay_ms <= 0) {
        return;
    }
#ifdef _WIN32
    Sleep((DWORD)delay_ms);
#else
    {
        struct timespec requested;
        requested.tv_sec = delay_ms / 1000;
        requested.tv_nsec = (long)(delay_ms % 1000) * 1000000L;
        nanosleep(&requested, NULL);
    }
#endif
}

void visualization_initialize_console(void) {
#ifdef _WIN32
    HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    SetConsoleOutputCP(CP_UTF8);
    if (output != INVALID_HANDLE_VALUE && GetConsoleMode(output, &mode)) {
        SetConsoleMode(output, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }
#endif
}

void visualization_clear_screen(void) {
    printf("\033[2J\033[H");
    fflush(stdout);
}

void visualization_draw_map(const Graph *graph, int start_id, int goal_id,
                            const int visited_order[], int visited_count,
                            int current_id, const PathResult *final_result) {
    ConsoleCanvas canvas;
    int x;
    int y;
    int i;

    if (graph == NULL) {
        return;
    }
    canvas_init(&canvas);
    for (i = 0; i < graph->edge_count; ++i) {
        canvas_draw_edge(&canvas, graph, &graph->edges[i], final_result);
    }
    for (i = 0; i < graph->node_count; ++i) {
        canvas_draw_node(&canvas, &graph->nodes[i], start_id, goal_id,
                         visited_order, visited_count, current_id, final_result);
    }

    printf("SCU Jiang'an Campus - Shortest Path Visualization\n");
    printf("Legend: %sSTART%s  %sGOAL%s  %sCURRENT%s  %sVISITED%s  %sPATH%s\n",
           COLOR_START, COLOR_RESET, COLOR_GOAL, COLOR_RESET,
           COLOR_CURRENT, COLOR_RESET, COLOR_VISITED, COLOR_RESET,
           COLOR_PATH, COLOR_RESET);
    printf("+");
    for (x = 0; x < CANVAS_WIDTH; ++x) putchar('-');
    printf("+\n");
    for (y = 0; y < CANVAS_HEIGHT; ++y) {
        printf("|");
        for (x = 0; x < CANVAS_WIDTH; ++x) {
            int node_id = canvas.node_ids[y][x];
            if (node_id >= 0) {
                printf("%s%c%s", node_color(node_id, start_id, goal_id, visited_order,
                                              visited_count, current_id, final_result),
                       canvas.cells[y][x], COLOR_RESET);
            } else if (canvas.path_cells[y][x]) {
                printf("%s%c%s", COLOR_PATH, canvas.cells[y][x], COLOR_RESET);
            } else if (canvas.cells[y][x] != ' ') {
                printf("%s%c%s", COLOR_ROAD, canvas.cells[y][x], COLOR_RESET);
            } else {
                putchar(' ');
            }
        }
        printf("|\n");
    }
    printf("+");
    for (x = 0; x < CANVAS_WIDTH; ++x) putchar('-');
    printf("+\n");

    for (i = 0; i < graph->node_count; i += 2) {
        printf("[%02d] %-24s", graph->nodes[i].id, graph->nodes[i].name);
        if (i + 1 < graph->node_count) {
            printf("[%02d] %-24s", graph->nodes[i + 1].id, graph->nodes[i + 1].name);
        }
        putchar('\n');
    }
}

void visualization_play_search(const Graph *graph, const PathResult *result,
                               int start_id, int goal_id, int delay_ms) {
    int frame;
    if (graph == NULL || result == NULL) {
        return;
    }
    for (frame = 0; frame < result->visited_count; ++frame) {
        visualization_clear_screen();
        visualization_draw_map(graph, start_id, goal_id, result->visited_order,
                               frame + 1, result->visited_order[frame], NULL);
        printf("\nSearching... visited %d/%d nodes\n", frame + 1, result->visited_count);
        fflush(stdout);
        sleep_ms(delay_ms);
    }
    visualization_clear_screen();
    visualization_draw_map(graph, start_id, goal_id, result->visited_order,
                           result->visited_count, -1, result);
    printf("\nSearch complete. Visited nodes: %d\n", result->visited_count);
    visualization_print_path(graph, result);
}

void visualization_print_graph(const Graph *graph) {
    int i;
    if (graph == NULL) {
        return;
    }
    printf("\nNodes (%d):\n", graph->node_count);
    for (i = 0; i < graph->node_count; ++i) {
        printf("  [%d] %-24s %-10s visible=%d (%.1f, %.1f)\n", graph->nodes[i].id,
               graph->nodes[i].name, storage_node_type_name(graph->nodes[i].type),
               graph->nodes[i].visible, graph->nodes[i].x, graph->nodes[i].y);
    }
    printf("Edges (%d):\n", graph->edge_count);
    for (i = 0; i < graph->edge_count; ++i) {
        printf("  %d <-> %d  distance=%.2f type=%s walkable=%d\n",
               graph->edges[i].from_id, graph->edges[i].to_id,
               graph->edges[i].weight, storage_road_type_name(graph->edges[i].type),
               graph->edges[i].walkable);
    }
}

void visualization_print_search(const Graph *graph, const PathResult *result) {
    int i;
    if (graph == NULL || result == NULL) {
        return;
    }
    printf("Visited: ");
    for (i = 0; i < result->visited_count; ++i) {
        const Node *node = graph_get_node(graph, result->visited_order[i]);
        printf("%s%s", node != NULL ? node->name : "?",
               i + 1 < result->visited_count ? " -> " : "\n");
    }
}

void visualization_print_path(const Graph *graph, const PathResult *result) {
    int i;
    if (graph == NULL || result == NULL || !result->found) {
        printf("No path found.\n");
        return;
    }
    printf("Shortest path: ");
    for (i = 0; i < result->path_length; ++i) {
        const Node *node = graph_get_node(graph, result->path[i]);
        printf("%s%s", node != NULL ? node->name : "?",
               i + 1 < result->path_length ? " -> " : "\n");
    }
    printf("Total distance: %.2f\n", result->total_distance);
    printf("Estimated walking time: %.0f minutes\n",
           result->total_distance > 0.8 ? result->total_distance * 1.25 : 1.0);
}
