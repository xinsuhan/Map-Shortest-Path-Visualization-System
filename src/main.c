#include "astar.h"
#include "dijkstra.h"
#include "graph.h"
#include "input.h"
#include "storage.h"
#include "visualization.h"

#ifdef MSP_HAS_3D
#include "renderer3d.h"
#include "renderer_phase1.h"
#endif

#include <stdio.h>
#include <string.h>

#define MSP_PATH_LENGTH 1024

typedef struct {
    int start_place_id;
    int goal_place_id;
    int start_id;
    int goal_id;
    int algorithm;
    int delay_ms;
    const char *speed_name;
} AppSettings;

static void reset_settings(AppSettings *settings) {
    settings->start_place_id = -1;
    settings->goal_place_id = -1;
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

static const char *selected_location_name(const Graph *graph, const PlaceStore *places,
                                          int place_id, int node_id) {
    const Place *place = storage_find_place(places, place_id);
    if (place != NULL) {
        return place->name;
    }
    return selected_node_name(graph, node_id);
}

static void wait_for_enter(void) {
    char buffer[8];
    input_read_string("\nPress Enter to continue...", buffer, sizeof(buffer));
}

static void show_main_screen(const Graph *graph, const PlaceStore *places,
                             const AppSettings *settings) {
    visualization_clear_screen();
    visualization_draw_map(graph, settings->start_id, settings->goal_id,
                           NULL, 0, -1, NULL);
    printf("\nCurrent settings\n");
    printf("  Start:     [%02d] %s\n", settings->start_id,
           selected_location_name(graph, places, settings->start_place_id,
                                  settings->start_id));
    printf("  Goal:      [%02d] %s\n", settings->goal_id,
           selected_location_name(graph, places, settings->goal_place_id,
                                  settings->goal_id));
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

static void print_places(const PlaceStore *places) {
    int i;
    if (places == NULL || places->place_count == 0) {
        return;
    }
    printf("\nPlaces:\n");
    for (i = 0; i < places->place_count; ++i) {
        const Place *place = &places->places[i];
        printf("  [%02d] %-28s entrance=%02d  %s\n", place->id, place->name,
               place->entrance_node_id, place->category);
    }
}

static int read_place_selection(const Graph *graph, const PlaceStore *places,
                                const char *prompt, int other_node_id,
                                int *selected_place_id, int *selected_node_id) {
    int place_id;
    const Place *place;
    print_places(places);
    if (!input_read_int(prompt, &place_id)) {
        printf("Invalid input: enter a numeric place ID.\n");
        return 0;
    }
    place = storage_find_place(places, place_id);
    if (place == NULL) {
        printf("Invalid place: ID %d does not exist.\n", place_id);
        return 0;
    }
    if (graph_get_node(graph, place->entrance_node_id) == NULL) {
        printf("Invalid place data: entrance node %d does not exist.\n",
               place->entrance_node_id);
        return 0;
    }
    if (place->entrance_node_id == other_node_id) {
        printf("Start and goal must use different entrance nodes.\n");
        return 0;
    }
    *selected_place_id = place_id;
    *selected_node_id = place->entrance_node_id;
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

static int direct_3d_requested(int argc, char *argv[]) {
    return argc == 2 && strcmp(argv[1], "--3d") == 0;
}

static int phase1_requested(int argc, char *argv[]) {
    return argc == 2 && strcmp(argv[1], "--phase1") == 0;
}

static void executable_directory(const char *argv0, char *directory, size_t size) {
    const char *slash;
    const char *backslash;
    const char *separator;
    size_t length;
    if (directory == NULL || size == 0) return;
    snprintf(directory, size, ".");
    if (argv0 == NULL) return;
    slash = strrchr(argv0, '/');
    backslash = strrchr(argv0, '\\');
    separator = slash;
    if (backslash != NULL && (separator == NULL || backslash > separator)) {
        separator = backslash;
    }
    if (separator == NULL) return;
    length = (size_t)(separator - argv0);
    if (length == 0) length = 1;
    if (length >= size) length = size - 1;
    memcpy(directory, argv0, length);
    directory[length] = '\0';
}

static int curved_data_exists(const char *directory) {
    char path[MSP_PATH_LENGTH];
    FILE *file;
    int written = snprintf(path, sizeof(path), "%s/data/curved/nodes.csv", directory);
    if (written < 0 || (size_t)written >= sizeof(path)) return 0;
    file = fopen(path, "r");
    if (file == NULL) return 0;
    fclose(file);
    return 1;
}

static int curved_data_path(char *path, size_t size, const char *base,
                            const char *file_name) {
    int written = snprintf(path, size, "%s/data/curved/%s", base, file_name);
    return written >= 0 && (size_t)written < size;
}

static int load_default_data(const char *argv0, Graph *graph, PlaceStore *places) {
    char executable_dir[MSP_PATH_LENGTH];
    char nodes[MSP_PATH_LENGTH];
    char edges[MSP_PATH_LENGTH];
    char geometry[MSP_PATH_LENGTH];
    char pois[MSP_PATH_LENGTH];
    const char *base = ".";

    executable_directory(argv0, executable_dir, sizeof(executable_dir));
    if (curved_data_exists(executable_dir)) base = executable_dir;
    if (!curved_data_path(nodes, sizeof(nodes), base, "nodes.csv") ||
        !curved_data_path(edges, sizeof(edges), base, "edges_curved.csv") ||
        !curved_data_path(geometry, sizeof(geometry), base,
                          "edge_geometry_points.csv") ||
        !curved_data_path(pois, sizeof(pois), base, "pois.csv")) {
        return MSP_ERROR_CAPACITY;
    }
    return storage_load_curved_campus(nodes, edges, geometry, pois, graph, places);
}

static int load_application_data(int argc, char *argv[], Graph *graph,
                                 PlaceStore *places) {
    int status;
    if (argc == 1 || direct_3d_requested(argc, argv)) {
        return load_default_data(argv[0], graph, places);
    }
    places->place_count = 0;
    if (argc >= 3) {
        status = storage_load_graph(argv[1], argv[2], graph);
    } else {
        status = storage_load_map(argv[1], graph);
    }
    if (status != MSP_OK) return status;
    if (argc >= 4) {
        return storage_load_places(argv[3], graph, places);
    }
    return MSP_OK;
}

int main(int argc, char *argv[]) {
    Graph graph;
    PlaceStore places;
    AppSettings settings;
    int choice;
    int load_status;
    int open_3d_directly = direct_3d_requested(argc, argv);

    if (phase1_requested(argc, argv)) {
#ifdef MSP_HAS_3D
        renderer_phase1_run();
        return 0;
#else
        fprintf(stderr, "Phase 1 viewer requires -DMSP_ENABLE_3D=ON.\n");
        return 1;
#endif
    }

    visualization_initialize_console();
    load_status = load_application_data(argc, argv, &graph, &places);
    if (load_status != MSP_OK) {
        fprintf(stderr, "Failed to load map data (status %d).\n", load_status);
        return 1;
    }
    reset_settings(&settings);

    if (open_3d_directly) {
#ifdef MSP_HAS_3D
        renderer3d_run(&graph, &places);
        return 0;
#else
        fprintf(stderr, "This build does not include 3D support. Reconfigure with "
                        "-DMSP_ENABLE_3D=ON.\n");
        return 1;
#endif
    }

    for (;;) {
        show_main_screen(&graph, &places, &settings);
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
            if (places.place_count > 0) {
                read_place_selection(&graph, &places, "Start place ID: ",
                                     settings.goal_id, &settings.start_place_id,
                                     &settings.start_id);
            } else {
                read_node_selection(&graph, "Start node ID: ", settings.goal_id,
                                    &settings.start_id);
            }
        } else if (choice == 2) {
            if (places.place_count > 0) {
                read_place_selection(&graph, &places, "Goal place ID: ",
                                     settings.start_id, &settings.goal_place_id,
                                     &settings.goal_id);
            } else {
                read_node_selection(&graph, "Goal node ID: ", settings.start_id,
                                    &settings.goal_id);
            }
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
            renderer3d_run(&graph, &places);
#endif
        } else {
#ifdef MSP_HAS_3D
            printf("Unknown option: choose a number from 0 to 8.\n");
#else
            printf("Unknown option: choose a number from 0 to 7.\n");
#endif
        }
        wait_for_enter();
    }

    visualization_clear_screen();
    printf("Goodbye.\n");
    return 0;
}
