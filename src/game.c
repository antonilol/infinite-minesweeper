#include "game.h"
#include "renderer.h"

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
