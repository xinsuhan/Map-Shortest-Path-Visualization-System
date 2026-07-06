#include "astar.h"
#include "dijkstra.h"
#include "input.h"
#include "storage.h"
#include "visualization.h"

#include <stdio.h>

static void run_algorithm(const Graph *graph, int algorithm) {
    int start_id;
    int goal_id;
    int status;
    PathResult result;

    if (!input_read_int("Start node ID: ", &start_id) ||
        !input_read_int("Goal node ID: ", &goal_id)) {
        printf("Invalid node ID.\n");
        return;
    }

    status = algorithm == 1
                 ? dijkstra_find_path(graph, start_id, goal_id, &result)
                 : astar_find_path(graph, start_id, goal_id, &result);
    if (status != MSP_OK) {
        printf("Path search failed (status %d).\n", status);
        return;
    }
    visualization_print_search(graph, &result);
    visualization_print_path(graph, &result);
}

int main(int argc, char *argv[]) {
    Graph graph;
    int choice;
    int load_status;

    if (argc >= 3) {
        load_status = storage_load_graph(argv[1], argv[2], &graph);
    } else {
        const char *map_path = argc == 2 ? argv[1] : "data/map.txt";
        load_status = storage_load_map(map_path, &graph);
    }
    if (load_status != MSP_OK) {
        fprintf(stderr, "Failed to load map data (status %d).\n", load_status);
        return 1;
    }

    for (;;) {
        printf("\n=== Map Shortest Path Visualization ===\n");
        printf("1. Show map\n2. Run Dijkstra\n3. Run A*\n0. Exit\n");
        if (!input_read_int("Select: ", &choice)) {
            printf("Please enter a number.\n");
            continue;
        }
        if (choice == 0) {
            break;
        }
        if (choice == 1) {
            visualization_print_graph(&graph);
        } else if (choice == 2 || choice == 3) {
            run_algorithm(&graph, choice == 2 ? 1 : 2);
        } else {
            printf("Unknown option.\n");
        }
    }
    return 0;
}
