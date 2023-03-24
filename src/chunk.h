#ifndef CHUNK_H
#define CHUNK_H

#include <stdbool.h>
#include <stdint.h>

#define BIT(n) (1 << n)
#define SET(flag, flags) ((flags) |= (flag))
#define UNSET(flag, flags) ((flags) &= ~(flag))
#define TOGGLE(flag, flags) ((flags) ^= (flag))
#define ISSET(flag, flags) (((flags) & (flag)) == (flag))

#define CHUNK_SIZE 64
#define CHUNK_LIST_SIZE 256
#define CHUNK_POS_MAX (CHUNK_SIZE - 1)

#define POS(x, y) ((x) + CHUNK_SIZE * (y))
#define NPOS(x, y) ((x) + 3 * (y))

// chunk flags
#define CHUNK_POPULATED 0x01
#define CHUNK_VISIBLE 0x02

// field flags
#define FIELD_MINE_CACHE_MASK 0x0f
#define FIELD_UNCOVERED 0x10
#define FIELD_MINE 0x20
#define FIELD_FLAG 0x40
#define FIELD_MINE_COUNT_CACHED 0x80

struct chunk {
	uint32_t x, y, seed;
	struct chunk *neighbors[9];
	uint8_t flags;
	uint8_t fields[CHUNK_SIZE * CHUNK_SIZE];
};

int alloc_chunk_list(const uint32_t size);

int push_chunk(struct chunk *c);

struct chunk *get_chunk_by_pos(const uint32_t x, const uint32_t y, const bool create);

struct chunk *create_chunk(const uint32_t x, const uint32_t y);

void check_covered_fields(struct chunk *c);

uint32_t xorshift32(uint32_t x);

void populate_chunk(struct chunk *c);

int is_mine(struct chunk *c, uint32_t x, uint32_t y);

struct chunk *get_neighbor(struct chunk *c, const uint32_t x, const uint32_t y);

int correct_pos(struct chunk **c, uint32_t *x, uint32_t *y);

int field_get_mines(struct chunk *c, const uint32_t x, const uint32_t y);

void uncover_field(struct chunk *c, uint32_t x, uint32_t y);

void field_toggle_flag(struct chunk *c, const uint32_t x, const uint32_t y);

#endif
