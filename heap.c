#include <assert.h>
#include <stdio.h>

#include "heap.h"

static const int DATA_SIZE = 8192;
static const int SECTION_SIZE = 64;

heap_t allocate_heap() {
    heap_t heap = (heap_t)malloc(sizeof(struct heap));
    if (heap == NULL) {
        perror("Cannot allocate heap:");
        exit(1);
    }

    uint32_t *data = (uint32_t *)malloc(DATA_SIZE * sizeof(uint32_t));
    if (data == NULL) {
        perror("Cannot allocate heap:");
        exit(1);
    }

    heap->data = data;
    heap->capacity = DATA_SIZE;

    heap_section_t sections =
        (heap_section_t)malloc(SECTION_SIZE * sizeof(struct heap_section));
    if (sections == NULL) {
        perror("Cannot allocate heap:");
        exit(1);
    }

    heap->sections_start = sections;
    heap->sections_end = sections + SECTION_SIZE;
    heap->current_section = sections;

    heap->current_section->start = 0;
    heap->current_section->end = 0;

    return heap;
}

void free_heap(heap_t heap) {
    free(heap->data);
    free(heap->sections_start);
    free(heap);
}

void heap_test(void) {
    heap_t heap = allocate_heap();

    for (int j = 0; j < 100; j++) {
        printf("Filling section %d\n", j);
        for (int i = 0; i < 10000; i++) {
            append_to_heap(heap, i);
        }
        printf("Verifying section %d.\n", j);
        printf("Section start: %d\n", heap->current_section->start);
        printf("Section end  : %d\n", heap->current_section->end);
        push_section(heap);
    }
    for (int j = 99; j >= 0; j--) {
        pop_section(heap);
        printf("Verifying section %d.\n", j);
        printf("Section start: %d\n", heap->current_section->start);
        printf("Section end  : %d\n", heap->current_section->end);
    }
}
