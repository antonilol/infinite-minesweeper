#ifndef GAME_H
#define GAME_H

#include <stdbool.h>
#include <stdint.h>

struct game {
	struct chunk **chunks;
	uint32_t chunks_count, chunks_size, mine_threshold, seed;
	int64_t view_x, view_y;
	int square_size;
  bool dirty, dead;
};

extern struct game *game;

void cleanup();

#endif
