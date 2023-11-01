#include "game.h"

#include "chunk.h"
#include "renderer.h"
#include "util.h"

#include <stdint.h>
#include <stdlib.h>

struct game *game;

void cleanup() {
	uint32_t i;

	cleanup_renderer();

	if (game->chunks) {
		for (i = 0; i < game->chunks_count; i++) {
			free(game->chunks[i]);
		}
		free(game->chunks);
	}

	free(game);
}

int init_game(const uint32_t seed) {
	game = calloc(1, sizeof(struct game));

	if (game == NULL) {
		handle_alloc_error();
	}

	alloc_chunk_list(CHUNK_LIST_SIZE);

	game->seed = seed;
	game->mine_threshold = DEFAULT_MINE_THRESHOLD;
	game->dirty = 1;
	game->square_size = SQUARE_SIZE_DEFAULT;

	return 0;
}
