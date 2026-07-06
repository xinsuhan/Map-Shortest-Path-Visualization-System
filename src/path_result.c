/* AI-assisted modification: added dynamic PathResult allocation support. */
#include "path_result.h"

#include <stdlib.h>
#include <string.h>

PathResult *path_result_create(void) {
    PathResult *result = (PathResult *)malloc(sizeof(*result));
    if (result == NULL) {
        return NULL;
    }
    memset(result, 0, sizeof(*result));
    return result;
}

void path_result_destroy(PathResult *result) {
    free(result);
}
