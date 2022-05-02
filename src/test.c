#ifdef TEST

#include "renderer.h"
#include "game.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

struct test_case1 {
  int x, y;
};

struct test_case2 {
  uint32_t cx, cy, fx, fy;
};

struct test_case1 *random_case1() {
  static struct test_case1 test;

  test.x = rand();
  test.y = rand();

  return &test;
}

int test1(struct test_case1 *test) {
  uint32_t cx, cy, fx, fy;
  int x, y;
  printf(" -> x=%d y=%d\n", test->x, test->y);
  screen_to_game(test->x, test->y, &cx, &cy, &fx, &fy);
  printf("    cx=%u cy=%u fx=%u fy=%u\n", cx, cy, fx, fy);
  game_to_screen(cx, cy, fx, fy, &x, &y);
  printf(" <- x=%d y=%d\n\n", x, y);
  return test->x - x <= 16 && test->x - x > -16 &&
         test->y - y <= 16 && test->y - y > -16;
}

int test2(struct test_case2 *test) {
  uint32_t cx, cy, fx, fy;
  int x, y;
  game_to_screen(test->cx, test->cy, test->fx, test->fy, &x, &y);
  screen_to_game(x, y, &cx, &cy, &fx, &fy);
  return test->cx == cx && test->cy == cy && test->fx == fx && test->fy == fy;
}

struct test_case1 cases1[] = {
  { 0, 0 },
  { 1, 1 },
  { -1, -1 },
  { INT32_MAX, INT32_MAX },
  { INT32_MIN, INT32_MIN }
};

int main() {
  int i;

  if (init_game(0)) {
    return 1;
  }

  srand(time(NULL));

  for (i = 0; i < sizeof(cases1) / sizeof(cases1[0]); i++) {
    if (!test1(&cases1[i])) {
      printf("fail\n");
      //return 1;
    }
  }

  return 0;
}

#endif
