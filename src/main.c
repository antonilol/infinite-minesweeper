#include "renderer.h"
#include "chunk.h"
#include "game.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define DEFAULT_MINE_THRESHOLD (-1U / 10 * 9)

int main(int argc, char **argv) {
	int i, err;
	uint32_t seed;
	uint64_t temp;

	if (argc > 1) {
		err = 1;
		// UINT32_MAX string length = 10
		for (i = 0; i < 10; i++) {
			if (argv[1][i] < '0' || argv[1][i] > '9') {
				printf("Invalid argument, please input a positive number\n");
				return 1;
			}
			if (argv[1][i + 1] == '\0') {
				err = 0;
				break;
			}
		}
		if (err) {
			printf("Number too large\n");
			return 1;
		}
		temp = strtoul(argv[1], NULL, 10);
		if (temp > UINT32_MAX) {
			printf("Number too large\n");
			return 1;
		}
		seed = temp;
		printf("Using seed %u\n", seed);
	} else {
		seed = time(NULL);
		printf("No seed set, using %u (time)\n", seed);
	}

	game = calloc(1, sizeof(struct game));

	if (game == NULL || alloc_chunk_list(CHUNK_LIST_SIZE)) {
		return 1;
	}
	game->seed = seed;
	game->mine_threshold = DEFAULT_MINE_THRESHOLD;
	game->dirty = 1;
	game->square_size = SQUARE_SIZE_DEFAULT;

	start_renderer();

	return 0;
}
