#include "chunk.h"
#include "game.h"
#include "renderer.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

int alloc_chunk_list(const uint32_t size) {
	struct chunk **new_chunks;

	if (game->chunks) {
		new_chunks = realloc(game->chunks, sizeof(game->chunks) * size);
	} else {
		new_chunks = malloc(sizeof(game->chunks) * size);
	}

	if (new_chunks == NULL) {
		return -1;
	}

	game->chunks = new_chunks;
	game->chunks_size = size;

	return 0;
}

int push_chunk(struct chunk *c) {
	// should never be greater, only equal
	if (game->chunks_count >= game->chunks_size) {
		if (alloc_chunk_list(game->chunks_size + CHUNK_LIST_SIZE)) {
			return -1;
		}
	}

	game->chunks[game->chunks_count++] = c;

	return 0;
}

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

struct chunk *create_chunk(const uint32_t x, const uint32_t y) {
	struct chunk *c, *n;
	int i, j;

	c = calloc(1, sizeof(struct chunk));

	if (c == NULL) {
		return NULL;
	}

	c->x = x;
	c->y = y;

	c->seed = xorshift32(xorshift32(xorshift32(x) ^ y) ^ game->seed) | 1;

	// link neighbors
	for (i = 0; i < 3; i++) {
		for (j = 0; j < 3; j++) {
			if (i != 1 || j != 1) {
				n = get_chunk_by_pos(x + j - 1, y + i - 1, false);
				if (n != NULL) {
					n->neighbors[8 - (i * 3 + j)] = c;
				}
				c->neighbors[i * 3 + j] = n;
			}
		}
	}

	push_chunk(c);

	return c;
}

static void uncover_field_inbounds_recalcuate(struct chunk *c, uint32_t x, uint32_t y);

void check_covered_fields(struct chunk *c) {
	uint32_t i;

	if (game->dead) {
		return;
	}

	// left top
	if (c->neighbors[NPOS(0, 0)] &&
		ISSET(FIELD_UNCOVERED,
			  c->neighbors[NPOS(0, 0)]->fields[POS(CHUNK_POS_MAX, CHUNK_POS_MAX)]) &&
		field_get_mines(c->neighbors[NPOS(0, 0)], CHUNK_POS_MAX, CHUNK_POS_MAX) == 0) {
		uncover_field_inbounds_recalcuate(c, 0, 0);
	}
	// right top
	if (c->neighbors[NPOS(2, 0)] &&
		ISSET(FIELD_UNCOVERED, c->neighbors[NPOS(2, 0)]->fields[POS(0, CHUNK_POS_MAX)]) &&
		field_get_mines(c->neighbors[NPOS(2, 0)], 0, CHUNK_POS_MAX) == 0) {
		uncover_field_inbounds_recalcuate(c, CHUNK_POS_MAX, 0);
	}
	// left bottom
	if (c->neighbors[NPOS(0, 2)] &&
		ISSET(FIELD_UNCOVERED, c->neighbors[NPOS(0, 2)]->fields[POS(CHUNK_POS_MAX, 0)]) &&
		field_get_mines(c->neighbors[NPOS(0, 2)], CHUNK_POS_MAX, 0) == 0) {
		uncover_field_inbounds_recalcuate(c, 0, CHUNK_POS_MAX);
	}
	// right bottom
	if (c->neighbors[NPOS(2, 2)] &&
		ISSET(FIELD_UNCOVERED, c->neighbors[NPOS(2, 2)]->fields[POS(0, 0)]) &&
		field_get_mines(c->neighbors[NPOS(2, 2)], 0, 0) == 0) {
		uncover_field_inbounds_recalcuate(c, CHUNK_POS_MAX, CHUNK_POS_MAX);
	}
	for (i = 0; i < CHUNK_SIZE; i++) {
		// top
		if (c->neighbors[NPOS(1, 0)] &&
			ISSET(FIELD_UNCOVERED, c->neighbors[NPOS(1, 0)]->fields[POS(i, CHUNK_POS_MAX)]) &&
			field_get_mines(c->neighbors[NPOS(1, 0)], i, CHUNK_POS_MAX) == 0) {
			uncover_field_inbounds_recalcuate(c, i, 0);
		}
		// left
		if (c->neighbors[NPOS(0, 1)] &&
			ISSET(FIELD_UNCOVERED, c->neighbors[NPOS(0, 1)]->fields[POS(CHUNK_POS_MAX, i)]) &&
			field_get_mines(c->neighbors[NPOS(0, 1)], CHUNK_POS_MAX, i) == 0) {
			uncover_field_inbounds_recalcuate(c, 0, i);
		}
		// right
		if (c->neighbors[NPOS(2, 1)] &&
			ISSET(FIELD_UNCOVERED, c->neighbors[NPOS(2, 1)]->fields[POS(0, i)]) &&
			field_get_mines(c->neighbors[NPOS(2, 1)], 0, i) == 0) {
			uncover_field_inbounds_recalcuate(c, CHUNK_POS_MAX, i);
		}
		// bottom
		if (c->neighbors[NPOS(1, 2)] &&
			ISSET(FIELD_UNCOVERED, c->neighbors[NPOS(1, 2)]->fields[POS(i, 0)]) &&
			field_get_mines(c->neighbors[NPOS(1, 2)], i, 0) == 0) {
			uncover_field_inbounds_recalcuate(c, i, CHUNK_POS_MAX);
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

int is_mine(struct chunk *c, uint32_t x, uint32_t y) {
	if (correct_pos(&c, &x, &y)) {
		return -1;
	}

	populate_chunk(c);

	return ISSET(FIELD_MINE, c->fields[POS(x, y)]);
}

struct chunk *get_neighbor(struct chunk *c, const uint32_t x, const uint32_t y) {
	struct chunk *n;

	if (c->neighbors[SNPOS(x, y)] == NULL) {
		n = create_chunk(c->x + x, c->y + y);
	} else {
		n = c->neighbors[SNPOS(x, y)];
	}

	if (c->neighbors[SNPOS(x, y)] == NULL) {
		printf("BUG: neighbor %d,%d not linked to %d,%d\n", c->x + x, c->y + y, c->x, c->y);
	}

	if (!is_visible(c) && !is_visible(n)) {
		SET(CHUNK_HIT, n->flags);
		return NULL;
	}

	return n;
}

int correct_pos(struct chunk **c, uint32_t *x, uint32_t *y) {
	struct chunk *n;
	int ox, oy;

	ox = *x == CHUNK_SIZE;
	if (*x > CHUNK_SIZE) {
		ox = -1;
	}
	oy = *y == CHUNK_SIZE;
	if (*y > CHUNK_SIZE) {
		oy = -1;
	}

	if (ox || oy) {
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

	for (i = 0; i < 3; i++) {
		for (j = 0; j < 3; j++) {
			if (i != 1 || j != 1) {
				m = is_mine(c, x + j - 1, y + i - 1);
				if (m == -1) {
					return -1;
				} else if (m) {
					mines++;
				}
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

	uncover_field_inbounds_recalcuate(c, x, y);
}

static void uncover_field(struct chunk *c, uint32_t x, uint32_t y) {
	if (game->dead) {
		return;
	}

	if (correct_pos(&c, &x, &y)) {
		return;
	}

	if (ISSET(FIELD_UNCOVERED, c->fields[POS(x, y)])) {
		return;
	}

	uncover_field_inbounds_recalcuate(c, x, y);
}

static void uncover_field_inbounds_recalcuate(struct chunk *c, uint32_t x, uint32_t y) {
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
	if (mines == 0) {
		for (i = 0; i < 3; i++) {
			for (j = 0; j < 3; j++) {
				if (i != 1 || j != 1) {
					uncover_field(c, x + j - 1, y + i - 1);
				}
			}
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
