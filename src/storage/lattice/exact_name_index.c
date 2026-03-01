/*
 * Exact-name index: hash table name -> node_id.
 * Open addressing, power-of-2 capacity, FNV-1a hash.
 */

#include "exact_name_index.h"
#include <stdlib.h>
#include <string.h>

#define INITIAL_CAPACITY 1024
#define MAX_LOAD_NUM 3
#define MAX_LOAD_DENOM 4

struct slot {
    char name[EXACT_NAME_MAX_LEN];
    uint64_t id;
};

struct exact_name_index_t {
    struct slot* slots;
    uint32_t capacity;
    uint32_t count;
};

static uint32_t hash_name(const char* name) {
    uint32_t h = 2166136261u;
    for (size_t i = 0; i < EXACT_NAME_MAX_LEN && name[i] != '\0'; i++) {
        h ^= (unsigned char)name[i];
        h *= 16777619u;
    }
    return h;
}

static void copy_name(char* dst, const char* src) {
    size_t i = 0;
    while (i < EXACT_NAME_MAX_LEN - 1 && src[i] != '\0') {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

static bool slot_empty(const struct slot* s) {
    return s->id == 0;
}

static bool slot_matches(const struct slot* s, const char* name) {
    return s->id != 0 && strcmp(s->name, name) == 0;
}

exact_name_index_t* exact_name_index_create(void) {
    exact_name_index_t* index = (exact_name_index_t*)malloc(sizeof(exact_name_index_t));
    if (!index) return NULL;
    index->capacity = INITIAL_CAPACITY;
    index->count = 0;
    index->slots = (struct slot*)calloc(index->capacity, sizeof(struct slot));
    if (!index->slots) {
        free(index);
        return NULL;
    }
    return index;
}

void exact_name_index_cleanup(exact_name_index_t* index) {
    if (!index) return;
    if (index->slots) {
        free(index->slots);
        index->slots = NULL;
    }
    index->capacity = 0;
    index->count = 0;
}

void exact_name_index_destroy(exact_name_index_t* index) {
    exact_name_index_cleanup(index);
    free(index);
}

uint64_t exact_name_index_get(exact_name_index_t* index, const char* name) {
    if (!index || !index->slots || !name) return 0;
    uint32_t mask = index->capacity - 1;
    uint32_t i = hash_name(name) & mask;
    for (uint32_t n = 0; n < index->capacity; n++) {
        struct slot* s = &index->slots[i];
        if (slot_empty(s)) return 0;
        if (slot_matches(s, name)) return s->id;
        i = (i + 1) & mask;
    }
    return 0;
}

static int exact_name_index_grow(exact_name_index_t* index) {
    uint32_t old_cap = index->capacity;
    struct slot* old_slots = index->slots;
    index->capacity *= 2;
    index->slots = (struct slot*)calloc(index->capacity, sizeof(struct slot));
    if (!index->slots) {
        index->capacity = old_cap;
        index->slots = old_slots;
        return -1;
    }
    index->count = 0;
    uint32_t mask = index->capacity - 1;
    for (uint32_t j = 0; j < old_cap; j++) {
        if (old_slots[j].id == 0) continue;
        uint32_t i = hash_name(old_slots[j].name) & mask;
        while (!slot_empty(&index->slots[i]))
            i = (i + 1) & mask;
        copy_name(index->slots[i].name, old_slots[j].name);
        index->slots[i].id = old_slots[j].id;
        index->count++;
    }
    free(old_slots);
    return 0;
}

void exact_name_index_put(exact_name_index_t* index, const char* name, uint64_t node_id) {
    if (!index || !index->slots || !name || node_id == 0) return;
    if (index->count * MAX_LOAD_DENOM >= index->capacity * MAX_LOAD_NUM) {
        if (exact_name_index_grow(index) != 0) return;
    }
    uint32_t mask = index->capacity - 1;
    uint32_t i = hash_name(name) & mask;
    for (uint32_t n = 0; n < index->capacity; n++) {
        struct slot* s = &index->slots[i];
        if (slot_empty(s) || slot_matches(s, name)) {
            copy_name(s->name, name);
            s->id = node_id;
            if (slot_empty(s)) index->count++;
            return;
        }
        i = (i + 1) & mask;
    }
}

void exact_name_index_remove(exact_name_index_t* index, const char* name) {
    if (!index || !index->slots || !name) return;
    uint32_t mask = index->capacity - 1;
    uint32_t i = hash_name(name) & mask;
    for (uint32_t n = 0; n < index->capacity; n++) {
        struct slot* s = &index->slots[i];
        if (slot_empty(s)) return;
        if (slot_matches(s, name)) {
            s->id = 0;
            s->name[0] = '\0';
            index->count--;
            return;
        }
        i = (i + 1) & mask;
    }
}

static void exact_name_index_clear(exact_name_index_t* index) {
    if (!index || !index->slots) return;
    memset(index->slots, 0, index->capacity * sizeof(struct slot));
    index->count = 0;
}

void exact_name_index_build(exact_name_index_t* index,
                            const char** node_names,
                            const uint64_t* node_ids,
                            uint32_t node_count) {
    if (!index || !node_names || !node_ids) return;
    exact_name_index_clear(index);
    if (!index->slots) return;
    while (index->capacity < node_count * 2) {
        if (exact_name_index_grow(index) != 0) return;
    }
    for (uint32_t k = 0; k < node_count; k++) {
        if (node_ids[k] == 0) continue;
        exact_name_index_put(index, node_names[k], node_ids[k]);
    }
}
