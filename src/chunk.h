#ifndef CHUNK_H
#define CHUNK_H

#include <stdbool.h>
#include <stdint.h>

#define CHUNK_SIZE_2LOG 6
#define CHUNK_SIZE (1 << CHUNK_SIZE_2LOG)
#define CHUNK_LIST_SIZE 256
#define CHUNK_POS_MAX (CHUNK_SIZE - 1)

// Get chunk index from its x and y
#define POS(x, y) ((x) + CHUNK_SIZE * (y))
// Get neightbor index from x and y, coordinates must be between -1 and 1, inclusive
#define NPOS(x, y) ((x) + 3 * (y) + 4)

// chunk flags
#define CHUNK_POPULATED 0x01
#define CHUNK_HIT 0x02

// field flags
#define FIELD_MINE_CACHE_MASK 0x0f
#define FIELD_UNCOVERED 0x10
#define FIELD_MINE 0x20
#define FIELD_FLAG 0x40
#define FIELD_MINE_COUNT_CACHED 0x80

struct chunk {
	uint8_t fields[CHUNK_SIZE * CHUNK_SIZE];
	struct chunk *neighbors[9];
	uint32_t x, y, seed;
	uint8_t flags;
};

void alloc_chunk_list(const uint32_t size);

struct chunk *get_chunk_by_pos(const uint32_t x, const uint32_t y, const bool create);

void check_covered_fields(struct chunk *c);

struct chunk *get_neighbor(struct chunk *c, const int32_t x, const int32_t y);

int field_get_mines(struct chunk *c, const uint32_t x, const uint32_t y);

void uncover_field_inbounds(struct chunk *c, uint32_t x, uint32_t y);

void field_toggle_flag(struct chunk *c, const uint32_t x, const uint32_t y);

#endif
