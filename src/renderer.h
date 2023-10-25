#ifndef RENDERER_H
#define RENDERER_H

#include "chunk.h"

#define WINDOW_TITLE "Infinite Minesweeper"
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

#define SQUARE_SIZE_MIN 16
#define SQUARE_SIZE_DEFAULT SQUARE_SIZE_MIN
#define SQUARE_SIZE_STEP 16
#define SQUARE_SIZE_MAX 64
#define TEXTURE_SIZE 16
#define TEXTURES 14
#define TEXTURES_FILE "assets/minesweeper.png"

// 0 has no number, it has no surrounding mines
// 1-8 are the numbers 1-8
#define TEXTURE_MINE 9
#define TEXTURE_MINE_HIT 10
#define TEXTURE_COVERED 11
#define TEXTURE_FLAG 12
#define TEXTURE_FLAG_WRONG 13

void cleanup_renderer();

int is_visible(struct chunk *c);

void start_renderer();

#endif
