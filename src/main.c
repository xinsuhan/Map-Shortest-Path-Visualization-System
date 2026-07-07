#include "astar.h"
#include "dijkstra.h"
#include "graph.h"
#include "input.h"
#include "storage.h"
#include "visualization.h"

#ifdef MSP_HAS_3D
#include "renderer3d.h"
#endif

#include <stdio.h>

typedef struct {
    int start_id;
    int goal_id;
    int algorithm;
    int delay_ms;
    const char *speed_name;
} AppSettings;

static void reset_settings(AppSettings *settings) {
    settings->start_id = -1;
    settings->goal_id = -1;
    settings->algorithm = 1;
    settings->delay_ms = 500;
    settings->speed_name = "Normal";
}

static const char *selected_node_name(const Graph *graph, int node_id) {
    const Node *node = graph_get_node(graph, node_id);
    return node != NULL ? node->name : "Not selected";
}

static void wait_for_enter(void) {
    char buffer[8];
    input_read_string("\nPress Enter to continue...", buffer, sizeof(buffer));
}

static void show_main_screen(const Graph *graph, const AppSettings *settings) {
    visualization_clear_screen();
    visualization_draw_map(graph, settings->start_id, settings->goal_id,
                           NULL, 0, -1, NULL);
    printf("\nCurrent settings\n");
    printf("  Start:     [%02d] %s\n", settings->start_id,
           selected_node_name(graph, settings->start_id));
    printf("  Goal:      [%02d] %s\n", settings->goal_id,
           selected_node_name(graph, settings->goal_id));
    printf("  Algorithm: %s\n", settings->algorithm == 1 ? "Dijkstra" : "A*");
    printf("  Speed:     %s (%d ms/frame)\n", settings->speed_name, settings->delay_ms);
    printf("\n1. Select start       2. Select goal\n");
    printf("3. Select algorithm   4. Select speed\n");
    printf("5. Start visualization\n");
    printf("6. Reset selections   7. Show map data\n");
#ifdef MSP_HAS_3D
    printf("8. Open 3D campus view\n");
#endif
    printf("0. Exit\n");
}

static int read_node_selection(const Graph *graph, const char *prompt, int other_id,
                               int *selected_id) {
    int node_id;
    if (!input_read_int(prompt, &node_id)) {
        printf("Invalid input: enter a numeric node ID.\n");
        return 0;
    }
    if (graph_get_node(graph, node_id) == NULL) {
        printf("Invalid node: ID %d does not exist.\n", node_id);
        return 0;
    }
    if (node_id == other_id) {
        printf("Start and goal must be different nodes.\n");
        return 0;
    }
    *selected_id = node_id;
    return 1;
}

static void select_algorithm(AppSettings *settings) {
    int choice;
    printf("\n1. Dijkstra\n2. A*\n");
    if (!input_read_int("Algorithm: ", &choice) || (choice != 1 && choice != 2)) {
        printf("Invalid algorithm selection.\n");
        return;
    }
    settings->algorithm = choice;
}

static void select_speed(AppSettings *settings) {
    int choice;
    printf("\n1. Slow (900 ms)\n2. Normal (500 ms)\n3. Fast (150 ms)\n");
    if (!input_read_int("Speed: ", &choice) || choice < 1 || choice > 3) {
        printf("Invalid speed selection.\n");
        return;
    }
    if (choice == 1) {
        settings->delay_ms = 900;
        settings->speed_name = "Slow";
    } else if (choice == 2) {
        settings->delay_ms = 500;
        settings->speed_name = "Normal";
    } else {
        settings->delay_ms = 150;
        settings->speed_name = "Fast";
    }
}

static void run_visualization(const Graph *graph, const AppSettings *settings) {
    PathResult result;
    int status;

    if (settings->start_id < 0 || settings->goal_id < 0) {
        printf("Select both a start node and a goal node first.\n");
        return;
    }
    status = settings->algorithm == 1
                 ? dijkstra_find_path(graph, settings->start_id, settings->goal_id, &result)
                 : astar_find_path(graph, settings->start_id, settings->goal_id, &result);
    if (status == MSP_ERROR_NO_PATH) {
        printf("No reachable path exists between the selected nodes.\n");
        return;
    }
    if (status != MSP_OK) {
        printf("Path search failed (status %d).\n", status);
        return;
    }
    visualization_play_search(graph, &result, settings->start_id,
                              settings->goal_id, settings->delay_ms);
}

static int load_map_from_arguments(int argc, char *argv[], Graph *graph) {
    if (argc >= 3) {
        return storage_load_graph(argv[1], argv[2], graph);
    }
    return storage_load_map(argc == 2 ? argv[1] : "data/map.txt", graph);
}

int main(int argc, char *argv[]) {
    Graph graph;
    AppSettings settings;
    int choice;
    int load_status;

    visualization_initialize_console();
    load_status = load_map_from_arguments(argc, argv, &graph);
    if (load_status != MSP_OK) {
        fprintf(stderr, "Failed to load map data (status %d).\n", load_status);
        return 1;
    }
    reset_settings(&settings);

    for (;;) {
        show_main_screen(&graph, &settings);
        if (!input_read_int("Select: ", &choice)) {
#ifdef MSP_HAS_3D
            printf("Invalid menu input: enter a number from 0 to 8.\n");
#else
            printf("Invalid menu input: enter a number from 0 to 7.\n");
#endif
            wait_for_enter();
            continue;
        }
        if (choice == 0) {
            break;
        }
        if (choice == 1) {
            read_node_selection(&graph, "Start node ID: ", settings.goal_id,
                                &settings.start_id);
        } else if (choice == 2) {
            read_node_selection(&graph, "Goal node ID: ", settings.start_id,
                                &settings.goal_id);
        } else if (choice == 3) {
            select_algorithm(&settings);
        } else if (choice == 4) {
            select_speed(&settings);
        } else if (choice == 5) {
            run_visualization(&graph, &settings);
        } else if (choice == 6) {
            reset_settings(&settings);
            printf("Selections and speed have been reset.\n");
        } else if (choice == 7) {
            visualization_print_graph(&graph);
#ifdef MSP_HAS_3D
        } else if (choice == 8) {
            renderer3d_run(&graph);
#endif
        } else {
            printf("Unknown option: choose a number from 0 to 7.\n");
        }
        wait_for_enter();
    }

    visualization_clear_screen();
    printf("Goodbye.\n");
    return 0;
}
