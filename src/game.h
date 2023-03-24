#ifndef GAME_H
#define GAME_H

#include <stdbool.h>
#include <stdint.h>

#define MINE_PERCENTAGE 10
#define DEFAULT_MINE_THRESHOLD (-1U / 100 * (100 - MINE_PERCENTAGE))

struct game {
	struct chunk **chunks;
	uint32_t chunks_count, chunks_size, mine_threshold, seed;
	int64_t view_x, view_y;
	int square_size;
	bool dirty, dead;
};

extern struct game *game;

void cleanup();

int init_game(const uint32_t seed);

#endif
