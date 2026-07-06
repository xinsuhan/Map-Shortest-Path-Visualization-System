#include "input.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int input_read_string(const char *prompt, char *buffer, size_t buffer_size) {
    size_t length;
    if (buffer == NULL || buffer_size == 0) {
        return 0;
    }
    if (prompt != NULL) {
        printf("%s", prompt);
    }
    if (fgets(buffer, (int)buffer_size, stdin) == NULL) {
        return 0;
    }
    length = strlen(buffer);
    if (length > 0 && buffer[length - 1] == '\n') {
        buffer[length - 1] = '\0';
    }
    return 1;
}

int input_read_int(const char *prompt, int *value) {
    char buffer[64];
    char *end;
    long parsed;

    if (value == NULL || !input_read_string(prompt, buffer, sizeof(buffer))) {
        return 0;
    }
    parsed = strtol(buffer, &end, 10);
    if (end == buffer || *end != '\0') {
        return 0;
    }
    *value = (int)parsed;
    return 1;
}

