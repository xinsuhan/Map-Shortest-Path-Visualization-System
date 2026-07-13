#include "input.h"

#include <assert.h>
#include <limits.h>
#include <stdio.h>

int main(void) {
    int value = 0;

    assert(input_parse_int("42", &value) && value == 42);
    assert(input_parse_int("-7", &value) && value == -7);
    assert(!input_parse_int("", &value));
    assert(!input_parse_int("12x", &value));
    assert(!input_parse_int("3.5", &value));
    assert(!input_parse_int("999999999999999999999", &value));
    assert(!input_parse_int("1 2", &value));
    assert(!input_parse_int("1", NULL));
    assert(input_parse_int("2147483647", &value) == (INT_MAX == 2147483647));
    puts("test_input passed");
    return 0;
}
