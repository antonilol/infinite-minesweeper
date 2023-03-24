.PHONY: build web

SHELL:=bash -O globstar

BUILD_DIR=build
EXEC=minesweeper
OUT=$(BUILD_DIR)/$(EXEC)

CC_FLAGS=src/*.c -O3 -Wall

all:build web

build:
	mkdir -p $(BUILD_DIR)
	gcc $(CC_FLAGS) `pkgconf --libs sdl2 SDL2_image --cflags sdl2` -o $(OUT)

test:
	mkdir -p $(BUILD_DIR)
	gcc $(CC_FLAGS) -DTEST `pkgconf --libs sdl2 SDL2_image --cflags sdl2` -o $(OUT)_test
	$(OUT)_test

run:build
	$(OUT)

debug:
	mkdir -p $(BUILD_DIR)
	gcc $(CC_FLAGS) -g `pkgconf --libs sdl2 SDL2_image --cflags sdl2` -o $(OUT)
	gdb -ex run $(OUT)

web:
	mkdir -p web
	cp minesweeper.html web/index.html
	emcc $(CC_FLAGS) -sWASM=1 -sUSE_SDL=2 -sUSE_SDL_IMAGE=2 -s SDL2_IMAGE_FORMATS='["png"]' -s ASSERTIONS=1 -s ALLOW_MEMORY_GROWTH=1 --preload-file=assets -o web/index.js

run_web:web
	emrun web/index.html

all:build web

clean:
	rm -rf build web

format:
	clang-format -i -style="{ColumnLimit: 100, UseTab: Always, IndentWidth: 4, TabWidth: 4}" src/**/*.{c,h}
