#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static const int DEFAULT_SIZE = 64;

struct array {
    uint32_t size;
    uint32_t length;
    uint32_t data[];
};

typedef struct array *array_t;

static array_t alloc_array() {
    const size_t allocation =
        2 * sizeof(size_t) + DEFAULT_SIZE * sizeof(uint32_t);
    array_t a = malloc(allocation);
    if (a == NULL) {
        perror("Allocation error:");
        exit(1);
    }
    a->size = DEFAULT_SIZE;
    a->length = 0;
    return a;
}

static array_t append_to_array(array_t a, uint32_t d) {
    if (a->length == a->size) {
        const uint32_t new_size = 2 * a->size;
        const size_t allocation =
            2 * sizeof(size_t) + new_size * sizeof(uint32_t);
        printf("Reallocating with size %u.\n", new_size);
        a = realloc(a, allocation);
        if (a == NULL) {
            perror("Allocation error:");
            exit(1);
        }
        a->size = new_size;
    }
    a->data[a->length] = d;
    a->length++;
    return a;
}
