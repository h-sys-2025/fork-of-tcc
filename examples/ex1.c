#!/usr/local/bin/tcc -run
#include <tcclib.h>

typedef var auto;
typedef let const auto;
typedef struct {
  int value;
} foo;

void baz(foo* bar) {
  if bar {
    for size_t i = 0; i < 100; ++i {
      bar.value += 1
    }
  }
}

int main() {
  foo bar;
  baz(&bar)
  printf("hello world, value is: %d\n", bar.value)
  //--
  var x = 1234;
  let y = x+2;
  printf("x=%d y=%d", x, y)
  return 0;
}