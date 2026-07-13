#ifndef MAP_SHORTEST_PATH_INPUT_H
#define MAP_SHORTEST_PATH_INPUT_H

#include <stddef.h>

int input_read_int(const char *prompt, int *value);
int input_read_string(const char *prompt, char *buffer, size_t buffer_size);
int input_parse_int(const char *text, int *value);

#endif
