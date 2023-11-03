#include "chunk.h"

#include "game.h"
#include "renderer.h"
#include "util.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

void alloc_chunk_list(const uint32_t size) {
	struct chunk **new_chunks;

	if (game->chunks == NULL) {
		new_chunks = malloc(sizeof(game->chunks) * size);
	} else {
		new_chunks = realloc(game->chunks, sizeof(game->chunks) * size);
	}

	if (new_chunks == NULL) {
		handle_alloc_error();
	}

	game->chunks = new_chunks;
	game->chunks_size = size;
}

static void push_chunk(struct chunk *c) {
	// should never be greater, only equal
	if (game->chunks_count >= game->chunks_size) {
		alloc_chunk_list(game->chunks_size + CHUNK_LIST_SIZE);
	}

	game->chunks[game->chunks_count++] = c;
}

static struct chunk *create_chunk(const uint32_t x, const uint32_t y);

struct chunk *get_chunk_by_pos(const uint32_t x, const uint32_t y, const bool create) {
	uint32_t i;

	for (i = 0; i < game->chunks_count; i++) {
		if (game->chunks[i]->x == x && game->chunks[i]->y == y) {
			return game->chunks[i];
		}
	}

	if (create) {
		return create_chunk(x, y);
	}

	return NULL;
}

static uint32_t xorshift32(uint32_t x) {
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	return x;
}

static struct chunk *create_chunk(const uint32_t x, const uint32_t y) {
	struct chunk *c, *n;
	int i, j;

	c = calloc(1, sizeof(struct chunk));

	if (c == NULL) {
		handle_alloc_error();
	}

	c->x = x;
	c->y = y;

	c->seed = xorshift32(xorshift32(xorshift32(x) ^ y) ^ game->seed) | 1;

	// link neighbors
	for (i = -1; i <= 1; i++) {
		for (j = -1; j <= 1; j++) {
			if (i == 0 && j == 0) {
				continue;
			}
			n = get_chunk_by_pos(x + j, y + i, false);
			if (n != NULL) {
				n->neighbors[NPOS(-j, -i)] = c;
			}
			c->neighbors[NPOS(j, i)] = n;
		}
	}

	push_chunk(c);

	return c;
}

static void uncover_field_inbounds_recalculate(struct chunk *c, uint32_t x, uint32_t y);

void check_covered_fields(struct chunk *c) {
	uint32_t i;

	if (game->dead) {
		return;
	}

	// left top
	if (c->neighbors[NPOS(-1, -1)] &&
		ISSET(FIELD_UNCOVERED,
			  c->neighbors[NPOS(-1, -1)]->fields[POS(CHUNK_POS_MAX, CHUNK_POS_MAX)]) &&
		field_get_mines(c->neighbors[NPOS(-1, -1)], CHUNK_POS_MAX, CHUNK_POS_MAX) == 0) {
		uncover_field_inbounds_recalculate(c, 0, 0);
	}
	// right top
	if (c->neighbors[NPOS(1, -1)] &&
		ISSET(FIELD_UNCOVERED, c->neighbors[NPOS(1, -1)]->fields[POS(0, CHUNK_POS_MAX)]) &&
		field_get_mines(c->neighbors[NPOS(1, -1)], 0, CHUNK_POS_MAX) == 0) {
		uncover_field_inbounds_recalculate(c, CHUNK_POS_MAX, 0);
	}
	// left bottom
	if (c->neighbors[NPOS(-1, 1)] &&
		ISSET(FIELD_UNCOVERED, c->neighbors[NPOS(-1, 1)]->fields[POS(CHUNK_POS_MAX, 0)]) &&
		field_get_mines(c->neighbors[NPOS(-1, 1)], CHUNK_POS_MAX, 0) == 0) {
		uncover_field_inbounds_recalculate(c, 0, CHUNK_POS_MAX);
	}
	// right bottom
	if (c->neighbors[NPOS(1, 1)] &&
		ISSET(FIELD_UNCOVERED, c->neighbors[NPOS(1, 1)]->fields[POS(0, 0)]) &&
		field_get_mines(c->neighbors[NPOS(1, 1)], 0, 0) == 0) {
		uncover_field_inbounds_recalculate(c, CHUNK_POS_MAX, CHUNK_POS_MAX);
	}
	for (i = 0; i < CHUNK_SIZE; i++) {
		// top
		if (c->neighbors[NPOS(0, -1)] &&
			ISSET(FIELD_UNCOVERED, c->neighbors[NPOS(0, -1)]->fields[POS(i, CHUNK_POS_MAX)]) &&
			field_get_mines(c->neighbors[NPOS(0, -1)], i, CHUNK_POS_MAX) == 0) {
			uncover_field_inbounds_recalculate(c, i, 0);
		}
		// left
		if (c->neighbors[NPOS(-1, 0)] &&
			ISSET(FIELD_UNCOVERED, c->neighbors[NPOS(-1, 0)]->fields[POS(CHUNK_POS_MAX, i)]) &&
			field_get_mines(c->neighbors[NPOS(-1, 0)], CHUNK_POS_MAX, i) == 0) {
			uncover_field_inbounds_recalculate(c, 0, i);
		}
		// right
		if (c->neighbors[NPOS(1, 0)] &&
			ISSET(FIELD_UNCOVERED, c->neighbors[NPOS(1, 0)]->fields[POS(0, i)]) &&
			field_get_mines(c->neighbors[NPOS(1, 0)], 0, i) == 0) {
			uncover_field_inbounds_recalculate(c, CHUNK_POS_MAX, i);
		}
		// bottom
		if (c->neighbors[NPOS(0, 1)] &&
			ISSET(FIELD_UNCOVERED, c->neighbors[NPOS(0, 1)]->fields[POS(i, 0)]) &&
			field_get_mines(c->neighbors[NPOS(0, 1)], i, 0) == 0) {
			uncover_field_inbounds_recalculate(c, i, CHUNK_POS_MAX);
		}
	}
}

void populate_chunk(struct chunk *c) {
	uint32_t x, y, state;

	if (ISSET(CHUNK_POPULATED, c->flags)) {
		return;
	}

	state = c->seed;

	for (y = 0; y < CHUNK_SIZE; y++) {
		for (x = 0; x < CHUNK_SIZE; x++) {
			state = xorshift32(state);
			if (state > game->mine_threshold) {
				SET(FIELD_MINE, c->fields[POS(x, y)]);
			}
		}
	}

	SET(CHUNK_POPULATED, c->flags);
}

static int correct_pos(struct chunk **c, int32_t *x, int32_t *y);

int is_mine(struct chunk *c, int32_t x, int32_t y) {
	if (correct_pos(&c, &x, &y)) {
		return -1;
	}

	populate_chunk(c);

	return ISSET(FIELD_MINE, c->fields[POS(x, y)]);
}

struct chunk *get_neighbor(struct chunk *c, const int32_t x, const int32_t y) {
	struct chunk *n;

	if (c->neighbors[NPOS(x, y)] == NULL) {
		n = create_chunk(c->x + x, c->y + y);
	} else {
		n = c->neighbors[NPOS(x, y)];
	}

	// if (c->neighbors[NPOS(x, y)] == NULL) {
	// 	printf("BUG: neighbor %d,%d not linked to %d,%d\n", c->x + x, c->y + y, c->x, c->y);
	// }

	if (!is_visible(c) && !is_visible(n)) {
		SET(CHUNK_HIT, n->flags);
		return NULL;
	}

	return n;
}

static int correct_pos(struct chunk **c, int32_t *x, int32_t *y) {
	struct chunk *n;
	int32_t ox, oy;

	ox = *x >> CHUNK_SIZE_2LOG;
	oy = *y >> CHUNK_SIZE_2LOG;

	if (ox != 0 || oy != 0) {
		n = get_neighbor(*c, ox, oy);

		if (n == NULL) {
			return 1;
		}

		*c = n;
		*x += ox * -CHUNK_SIZE;
		*y += oy * -CHUNK_SIZE;
	}

	return 0;
}

int field_get_mines(struct chunk *c, const uint32_t x, const uint32_t y) {
	int m, mines, i, j;

	populate_chunk(c);

	if (ISSET(FIELD_MINE_COUNT_CACHED, c->fields[POS(x, y)])) {
		return c->fields[POS(x, y)] & FIELD_MINE_CACHE_MASK;
	}

	mines = 0;

	for (i = -1; i <= 1; i++) {
		for (j = -1; j <= 1; j++) {
			if (i == 0 && j == 0) {
				continue;
			}
			m = is_mine(c, x + j, y + i);
			if (m == -1) {
				return -1;
			} else if (m) {
				mines++;
			}
		}
	}

	SET(FIELD_MINE_COUNT_CACHED | mines, c->fields[POS(x, y)]);

	return mines;
}

void uncover_field_inbounds(struct chunk *c, uint32_t x, uint32_t y) {
	if (game->dead) {
		return;
	}

	if (ISSET(FIELD_UNCOVERED, c->fields[POS(x, y)])) {
		return;
	}

	uncover_field_inbounds_recalculate(c, x, y);
}

static void uncover_field(struct chunk *c, int32_t x, int32_t y) {
	if (game->dead) {
		return;
	}

	if (correct_pos(&c, &x, &y)) {
		return;
	}

	if (ISSET(FIELD_UNCOVERED, c->fields[POS(x, y)])) {
		return;
	}

	uncover_field_inbounds_recalculate(c, x, y);
}

static void uncover_field_inbounds_recalculate(struct chunk *c, uint32_t x, uint32_t y) {
	int mines, i, j;

	if (ISSET(FIELD_FLAG, c->fields[POS(x, y)])) {
		return;
	}

	SET(FIELD_UNCOVERED, c->fields[POS(x, y)]);
	if (ISSET(FIELD_MINE, c->fields[POS(x, y)])) {
		game->dead = 1;
		return;
	}

	mines = field_get_mines(c, x, y);
	// useless. if mines != 0 it will do nothing anyway
	// if (mines == -1) {
	// 	 return;
	// }
	if (mines != 0) {
		return;
	}
	for (i = -1; i <= 1; i++) {
		for (j = -1; j <= 1; j++) {
			if (i == 0 && j == 0) {
				continue;
			}
			uncover_field(c, x + j, y + i);
		}
	}
}

void field_toggle_flag(struct chunk *c, const uint32_t x, const uint32_t y) {
	if (game->dead) {
		return;
	}

	if (!ISSET(FIELD_UNCOVERED, c->fields[POS(x, y)])) {
		TOGGLE(FIELD_FLAG, c->fields[POS(x, y)]);
	}
}
