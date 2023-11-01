#include "renderer.h"

#include "chunk.h"
#include "game.h"
#include "util.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

static bool run;

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static SDL_Rect texture_srcrect = {0, 0, TEXTURE_SIZE, TEXTURE_SIZE},
				texture_dstrect = {0, 0, SQUARE_SIZE_DEFAULT, SQUARE_SIZE_DEFAULT};
static SDL_Texture *texture;

static int w, h;
static bool moving = false;

static int init_sdl() {
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		printf("%s\n", SDL_GetError());
		return 1;
	}

	SDL_CreateWindowAndRenderer(WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_RESIZABLE, &window,
								&renderer);
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
		printf("Textures file " TEXTURES_FILE " has wrong size\nActual: %dx%d, must be: %dx%d\n",
			   surface->w, surface->h, TEXTURE_SIZE, TEXTURE_SIZE * TEXTURES);
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

static void game_to_screen(const uint32_t cx, const uint32_t cy, const uint32_t fx,
						   const uint32_t fy, int *x, int *y) {
	if (x)
		*x = (((int64_t)cx) * CHUNK_SIZE + ((int64_t)fx)) * game->square_size + game->view_x;
	if (y)
		*y = (((int64_t)cy) * CHUNK_SIZE + ((int64_t)fy)) * game->square_size + game->view_y;
}

// int_div_round_down(a, 1 << s) == a >> s, where a is an int64_t and s a number valid for bit
// shifting an int64_t. The return value for values of b that are not powers of two is equivalent
// but can not be expressed using a bit shift.
static int64_t int_div_round_down(const int64_t a, const int64_t b) {
	return a / b - (a < 0 && a % b != 0);
}

static void screen_to_game(const int x, const int y, uint32_t *cx, uint32_t *cy, uint32_t *fx,
						   uint32_t *fy) {
	int64_t gfx, gfy;

	gfx = int_div_round_down(x - game->view_x, game->square_size);
	gfy = int_div_round_down(y - game->view_y, game->square_size);
	if (cx)
		*cx = gfx >> CHUNK_SIZE_2LOG;
	if (cy)
		*cy = gfy >> CHUNK_SIZE_2LOG;
	if (fx)
		*fx = ((uint32_t)gfx) % CHUNK_SIZE;
	if (fy)
		*fy = ((uint32_t)gfy) % CHUNK_SIZE;
}

int is_visible(struct chunk *c) {
	int x, y;
	bool visible;

	game_to_screen(c->x, c->y, 0, 0, &x, &y);

	visible = x + game->square_size * CHUNK_SIZE >= 0 && x < w &&
			  y + game->square_size * CHUNK_SIZE >= 0 && y < h;

	if (visible && ISSET(CHUNK_HIT, c->flags)) {
		UNSET(CHUNK_HIT, c->flags);

		check_covered_fields(c);
	}

	return visible;
}

static void render_field(const int x, const int y, const uint8_t field, const int mines) {
	if (x < -game->square_size || x > game->square_size + w || y < -game->square_size ||
		y > game->square_size + h) {
		return;
	}

	if (ISSET(FIELD_FLAG, field)) {
		if (game->dead && !ISSET(FIELD_MINE, field)) {
			texture_srcrect.y = TEXTURE_FLAG_WRONG * TEXTURE_SIZE;
		} else {
			texture_srcrect.y = TEXTURE_FLAG * TEXTURE_SIZE;
		}
	} else if (!game->dead && !ISSET(FIELD_UNCOVERED, field)) {
		texture_srcrect.y = TEXTURE_COVERED * TEXTURE_SIZE;
	} else if (ISSET(FIELD_MINE, field)) {
		if (ISSET(FIELD_UNCOVERED, field)) {
			texture_srcrect.y = TEXTURE_MINE_HIT * TEXTURE_SIZE;
		} else {
			texture_srcrect.y = TEXTURE_MINE * TEXTURE_SIZE;
		}
	} else {
		texture_srcrect.y = mines * TEXTURE_SIZE;
	}

	texture_dstrect.x = x;
	texture_dstrect.y = y;

	SDL_RenderCopy(renderer, texture, &texture_srcrect, &texture_dstrect);
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
	static struct chunk *c = NULL;
	uint32_t x, y;
	int32_t dx, dy;

	if (game->dirty) {
		SDL_GetWindowSize(window, &w, &h);

		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);

		screen_to_game(0, 0, &x, &y, NULL, NULL);

		if (c == NULL) {
			// first time rendering
			c = get_chunk_by_pos(x, y, true);
		} else {
			dx = x - c->x;
			dy = y - c->y;
			if (dx == 0 && dy == 0) {
				// no lookup, we got the right chunk already
				// most of the time the previous frame started with the same chunk
			} else if (dx >= -1 && dx <= 1 && dy >= -1 && dy <= 1) {
				// O(1) lookup, we moved less than one chunk between two frames
				// every time we move the screen origin (0, 0) over a chunk boundary
				c = c->neighbors[NPOS(dx, dy)];
			} else {
				// O(n) lookup, we moved *over* a chunk
				// is is super rare
				c = get_chunk_by_pos(x, y, true);
			}
		}

		render_chunk(c);

		SDL_RenderPresent(renderer);

		game->dirty = 0;
	}
}

static void main_loop() {
	static int mouseX, mouseY;

	struct chunk *c;
	SDL_Event event;
	uint32_t cx, cy, fx, fy;
	int prev_square_size, new_square_size;

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
					if (ISSET(FIELD_FLAG, c->fields[POS(fx, fy)])) {
						field_toggle_flag(c, fx, fy);
					} else {
						uncover_field_inbounds(c, fx, fy);
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
			mouseX = event.motion.x;
			mouseY = event.motion.y;
		} else if (event.type == SDL_MOUSEWHEEL) {
			prev_square_size = game->square_size;

			new_square_size =
				prev_square_size + event.wheel.y * SQUARE_SIZE_STEP *
									   (event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? -1 : 1);
			if (new_square_size > SQUARE_SIZE_MAX) {
				new_square_size = SQUARE_SIZE_MAX;
			} else if (new_square_size < SQUARE_SIZE_MIN) {
				new_square_size = SQUARE_SIZE_MIN;
			}

			game->view_x = mouseX - int_div_round_down(new_square_size * (mouseX - game->view_x),
													   prev_square_size);
			game->view_y = mouseY - int_div_round_down(new_square_size * (mouseY - game->view_y),
													   prev_square_size);

			texture_dstrect.w = texture_dstrect.h = new_square_size;
			game->square_size = new_square_size;
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
		SDL_Delay(10);
	}
#endif

error:
	cleanup();
}
