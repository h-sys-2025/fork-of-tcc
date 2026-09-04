#!/usr/local/bin/tcc -run
#include <tcclib.h>

typedef struct {
  int value;
} foo;

void baz(foo* bar) {
  if (bar) {
    for (size_t i = 0; i < 100; ++i) {
      bar.value += 1;
    }
  } else {
    printf("pointer expected, you forgot to add a '&'.");
  }
}

int main() {
  foo bar = {0};
  baz(bar);
  printf("hello world, value is: %d\n", bar.value);
  return 0;
}