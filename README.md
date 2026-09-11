# Tiny C Compiler - C Scripting Everywhere - The Smallest ANSI C compiler.

```c
#!/usr/local/bin/tcc -run
#include <tcclib.h>

typedef struct {
  int value;
} foo;

void baz(foo* bar) {
  if bar {
    for size_t i = 0; i < 100; ++i {
      bar.value += 1
    }
  } else {
    printf("pointer expected, you forgot to add a '&'.")
  }
}

int main() {
  foo bar;
  baz(&bar)
  printf("hello world, value is: %d\n", bar.value)
  //--
  auto x = 1234;
  printf("%d", x)
  return 0;
}}
```