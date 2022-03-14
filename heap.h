#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef HEAP_H
#define HEAP_H

#ifdef __cplusplus
extern "C" {
#endif

struct heap_section {
    int start;
    int end;
};

typedef struct heap_section *heap_section_t;

struct heap {
    uint32_t *data;
    int capacity;
    heap_section_t sections_start;
    heap_section_t sections_end;
    heap_section_t current_section;
};

typedef struct heap *heap_t;

heap_t allocate_heap();
void free_heap(heap_t heap);

static void append_to_heap(heap_t heap, uint32_t data) {
    if (heap->current_section->end >= heap->capacity) {
        // printf("Reallocating heap.\n");
        uint32_t *new_data = (uint32_t *)realloc(
            heap->data, 2 * heap->capacity * sizeof(uint32_t));
        if (new_data == NULL) {
            perror("Cannot grow heap.");
            exit(1);
        }
        heap->data = new_data;
        heap->capacity = 2 * heap->capacity;
        // printf("Grown heap capacity to %d.\n", heap->capacity);
    }
    heap->data[heap->current_section->end++] = data;
}

static void push_section(heap_t heap) {
    heap->current_section++;
    if (heap->current_section == heap->sections_end) {
        // printf("Reallocating sections.\n");
        int nsections = heap->sections_end - heap->sections_start;
        heap_section_t new_sections = (heap_section_t)realloc(
            heap->sections_start, 2 * nsections * sizeof(struct heap_section));
        if (new_sections == NULL) {
            perror("Cannot grow sections.");
            exit(1);
        }
        heap->sections_start = new_sections;
        heap->sections_end = new_sections + 2 * nsections;
        heap->current_section = new_sections + nsections;
    }
    heap->current_section->start = (heap->current_section - 1)->end;
    heap->current_section->end = heap->current_section->start;
}

static void pop_section(heap_t heap) {
    assert(heap->current_section > heap->sections_start);
    heap->current_section--;
}

#ifdef __cplusplus
}
#endif

#endif
