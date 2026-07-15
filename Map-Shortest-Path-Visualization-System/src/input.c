#include "input.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int input_read_string(const char *prompt, char *buffer, size_t buffer_size) {
    size_t length;
    int truncated = 0;
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
        buffer[--length] = '\0';
        if (length > 0 && buffer[length - 1] == '\r') {
            buffer[length - 1] = '\0';
        }
    } else if (!feof(stdin)) {
        int ch;
        truncated = 1;
        while ((ch = getchar()) != '\n' && ch != EOF) {
        }
    }
    return !truncated;
}

int input_parse_int(const char *text, int *value) {
    char *end;
    long parsed;

    if (text == NULL || value == NULL || text[0] == '\0') {
        return 0;
    }
    errno = 0;
    parsed = strtol(text, &end, 10);
    if (errno == ERANGE || end == text || *end != '\0' ||
        parsed < INT_MIN || parsed > INT_MAX) {
        return 0;
    }
    *value = (int)parsed;
    return 1;
}

int input_read_int(const char *prompt, int *value) {
    char buffer[64];
    if (value == NULL || !input_read_string(prompt, buffer, sizeof(buffer))) {
        return 0;
    }
    return input_parse_int(buffer, value);
}
