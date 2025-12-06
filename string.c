#include <stdio.h>

// LFFFFF

void ps_init(char *s, char *init, int len) {
  unsigned char *lenptr = s;
  *lenptr = len;
  for (int j = 0; j < len; j++) {
    s[j+1] = init[j];
  }
}

void ps_print(char *s) {

}

int main(void) {
  char buf[256];
  ps_init(buf, "Hello World", 11);
  ps_print(buf);
}
