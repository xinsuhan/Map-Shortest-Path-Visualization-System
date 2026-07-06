#include "path_result.h"

#include <assert.h>
#include <stdio.h>

int main(void) {
    int i;
    PathResult *result = path_result_create();

    assert(result != NULL);
    assert(result->found == 0);
    assert(result->total_distance == 0.0);
    assert(result->path_length == 0);
    assert(result->visited_count == 0);
    for (i = 0; i < MSP_MAX_NODES; ++i) {
        assert(result->path[i] == 0);
        assert(result->visited_order[i] == 0);
    }
    path_result_destroy(result);
    path_result_destroy(NULL);
    puts("test_path_result passed");
    return 0;
}
