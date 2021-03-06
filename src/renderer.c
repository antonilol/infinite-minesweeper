#include "renderer.h"
#include "game.h"
#include "chunk.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

static bool run;

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static SDL_Rect srcrect = { 0, 0, TEXTURE_SIZE, TEXTURE_SIZE },
								dstrect = { 0, 0, SQUARE_SIZE_DEFAULT, SQUARE_SIZE_DEFAULT };
static SDL_Texture *texture;

static int w, h;
static bool moving = false;

static int init_sdl() {
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		printf("%s\n", SDL_GetError());
		return 1;
	}

	SDL_CreateWindowAndRenderer(WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_RESIZABLE, &window, &renderer);
	if (window == NULL || renderer == NULL) {
		printf("%s\n", SDL_GetError());
		return 1;
	}

	SDL_SetWindowTitle(window, WINDOW_TITLE);

	return 0;
}

static int init_textures() {
	SDL_Surface *surface = IMG_Load(TEXTURES_FILE);
	if (!surface) {
		printf("%s\n", IMG_GetError());
		return 1;
	}
	if (surface->w != TEXTURE_SIZE || surface->h != TEXTURE_SIZE * TEXTURES) {
		printf("Textures file " TEXTURES_FILE " has wrong size\nActual: %dx%d, must be: %dx%d\n", surface->w, surface->h, TEXTURE_SIZE, TEXTURE_SIZE * TEXTURES);
		return 1;
	}
	texture = SDL_CreateTextureFromSurface(renderer, surface);
	SDL_FreeSurface(surface);
	return 0;
}

void cleanup_renderer() {
	if (renderer) {
		SDL_DestroyRenderer(renderer);
	}
	if (window) {
		SDL_DestroyWindow(window);
	}
	SDL_Quit();
}

void game_to_screen(const uint32_t cx, const uint32_t cy, const uint32_t fx, const uint32_t fy, int *x, int *y) {
	if (x) *x = (((int64_t) cx) * CHUNK_SIZE + ((int64_t) fx)) * game->square_size + game->view_x;
	if (y) *y = (((int64_t) cy) * CHUNK_SIZE + ((int64_t) fy)) * game->square_size + game->view_y;
}

static int64_t int_div(const int64_t a, const int64_t b) {
	return a / b - (a < 0);
}

void screen_to_game(const int x, const int y, uint32_t *cx, uint32_t *cy, uint32_t *fx, uint32_t *fy) {
	int64_t gfx, gfy;

	gfx = int_div(x - game->view_x, game->square_size);
	gfy = int_div(y - game->view_y, game->square_size);
	if (cx) *cx = int_div(gfx, CHUNK_SIZE);
	if (cy) *cy = int_div(gfy, CHUNK_SIZE);
	if (fx) *fx = ((uint32_t) gfx) % CHUNK_SIZE;
	if (fy) *fy = ((uint32_t) gfy) % CHUNK_SIZE;
}

int is_visible(struct chunk *c) {
	int x, y;
	bool visible;

	game_to_screen(c->x, c->y, 0, 0, &x, &y);

	visible = x + game->square_size * CHUNK_SIZE >= 0 && x < w && y + game->square_size * CHUNK_SIZE >= 0 && y < h;

	if (visible && !ISSET(CHUNK_VISIBLE, c->flags)) {
		SET(CHUNK_VISIBLE, c->flags);

		check_covered_fields(c);
	}

	return visible;
}

static void render_field(const int x, const int y, const uint8_t field, const int mines) {
	if (
		x < -game->square_size || x > game->square_size + w ||
		y < -game->square_size || y > game->square_size + h
	) {
		return;
	}

	if (ISSET(FIELD_FLAG, field)) {
		if (game->dead && !ISSET(FIELD_MINE, field)) {
			srcrect.y = TEXTURE_FLAG_WRONG * TEXTURE_SIZE;
		} else {
			srcrect.y = TEXTURE_FLAG * TEXTURE_SIZE;
		}
	} else if (!game->dead && !ISSET(FIELD_UNCOVERED, field)) {
		srcrect.y = TEXTURE_COVERED * TEXTURE_SIZE;
	} else if (ISSET(FIELD_MINE, field)) {
		if (ISSET(FIELD_UNCOVERED, field)) {
			srcrect.y = TEXTURE_MINE_HIT * TEXTURE_SIZE;
		} else {
			srcrect.y = TEXTURE_MINE * TEXTURE_SIZE;
		}
	} else {
		srcrect.y = mines * TEXTURE_SIZE;
	}

	dstrect.x = x;
	dstrect.y = y;

	SDL_RenderCopy(renderer, texture, &srcrect, &dstrect);
}

static void render_chunk(struct chunk *c) {
	struct chunk *next;
	int x, y, sx, sy;

	for (y = 0; y < CHUNK_SIZE; y++) {
		for (x = 0; x < CHUNK_SIZE; x++) {
			game_to_screen(c->x, c->y, x, y, &sx, &sy);
			render_field(sx, sy, c->fields[POS(x, y)], field_get_mines(c, x, y));
		}
	}

	// debug chunk borders
	// game_to_screen(c->x, c->y, 0, 0, &sx, &sy);
	// SDL_SetRenderDrawColor(renderer, (c->x || c->y) * 0xff, 0, 0, 0xff);
	// SDL_RenderDrawLine(renderer, sx, sy, sx + game->square_size * CHUNK_SIZE, sy);
	// SDL_RenderDrawLine(renderer, sx, sy, sx, sy + game->square_size * CHUNK_SIZE);

	next = get_neighbor(c, 1, 0);
	if (next) {
		render_chunk(next);
	}
	next = get_neighbor(c, 0, 1);
	if (next) {
		render_chunk(next);
	}
}

static void render() {
	struct chunk *c;
	uint32_t x, y;

	if (game->dirty) {
		SDL_GetWindowSize(window, &w, &h);

		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);

		screen_to_game(0, 0, &x, &y, NULL, NULL);

		c = get_chunk_by_pos(x, y, true);

		render_chunk(c);

		SDL_RenderPresent(renderer);

		game->dirty = 0;
	}
}

static void main_loop() {
	struct chunk *c;
	SDL_Event event;
	uint32_t cx, cy, fx, fy;

	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_QUIT) {
			run = 0;
			break;
		} else if (event.type == SDL_WINDOWEVENT) {
			game->dirty = 1;
    } else if (event.type == SDL_MOUSEBUTTONUP) {
			if (event.button.button == SDL_BUTTON_LEFT) {
				if (moving) {
					moving = false;
				} else {
					screen_to_game(event.button.x, event.button.y, &cx, &cy, &fx, &fy);
					c = get_chunk_by_pos(cx, cy, true);
					if (ISSET(FIELD_FLAG, c->fields[fy * CHUNK_SIZE + fx])) {
						field_toggle_flag(c, fx, fy);
					} else {
						uncover_field(c, fx, fy);
					}
					game->dirty = 1;
				}
			} else if (event.button.button == SDL_BUTTON_RIGHT) {
				screen_to_game(event.button.x, event.button.y, &cx, &cy, &fx, &fy);
				field_toggle_flag(get_chunk_by_pos(cx, cy, true), fx, fy);
				game->dirty = 1;
			}
		} else if (event.type == SDL_MOUSEMOTION) {
			if (ISSET(SDL_BUTTON_LMASK, event.motion.state)) {
				moving = true;
				game->view_x += event.motion.xrel;
				game->view_y += event.motion.yrel;
				game->dirty = 1;
			}
		} else if (event.type == SDL_MOUSEWHEEL) {
			game->square_size += event.wheel.y * SQUARE_SIZE_STEP * (event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? -1 : 1);
			if (game->square_size > SQUARE_SIZE_MAX) {
				game->square_size = SQUARE_SIZE_MAX;
			} else if (game->square_size < SQUARE_SIZE_MIN) {
				game->square_size = SQUARE_SIZE_MIN;
			}
			dstrect.w = dstrect.h = game->square_size;
			game->dirty = 1;
		}
	}

	render();

#ifdef __EMSCRIPTEN__
	if (!run) {
		emscripten_cancel_main_loop();
		cleanup();
	}
#endif
}

void start_renderer() {
	if (init_sdl()) {
		goto error;
	}

	if (init_textures()) {
		goto error;
	}

	run = 1;

#ifdef __EMSCRIPTEN__
	emscripten_set_main_loop(main_loop, -1, 1);
	return;
#else
	while (run) {
		main_loop();
	}
#endif

error:
	cleanup();
}
