# Infinite Minesweeper

(Still in beta, it has known bugs)

Play at https://antonilol.github.io/infinite-minesweeper/

### Controls

**PC:**
<br>
Left mouse button: uncover a field, hold and move your mouse to move the game
<br>
Right mouse button: toggle flag at a covered field
<br>
Refresh or reopen to restart the game

**Mobile:**
<br>
No touch controls implemented yet (coming soon)

### Building from source

Clone this repository:

```
git clone https://github.com/antonilol/infinite-minesweeper.git
cd infinite-minesweeper
```

Install SDL2 and SDL2 Image with your distro's package manager

Arch based distros:
```
sudo pacman -S sdl2 sdl2_image
```

Ubuntu based distros:
```
sudo apt install libsdl2-dev libsdl2-image-dev
```

#### Native (on Linux)

Make sure you have pkgconf installed

```
make
```

The compiled binary can be found in `build/minesweeper`

#### WebAssembly

Make sure you have emscripten installed, on Arch you can get almost the newest one with pacman,
but apt tends to have a lot of outdated packages. You can install emscripten [here](https://emscripten.org/docs/getting_started/downloads.html)

```
make web
```

The compiled WASM binary and other needed files can be found in `web`

`make all` will compile both native and webassembly

### Pre built (WASM only)

https://antonilol.github.io/infinite-minesweeper/ is built for every commit in this repository,
its files can found on the [gh-pages branch](https://github.com/antonilol/infinite-minesweeper/tree/gh-pages).

### Developing

PRs are welcome, but please discuss large changes with me first.

To test changes you can run `make run`, this will compile and run.

To test in the browser, run `make run_web`, this will fire up a web server locally because
browsers can't make requests to files on your disk but can to a web server.
