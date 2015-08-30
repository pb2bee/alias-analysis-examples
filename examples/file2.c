#include <stdlib.h>

int* maxPointer(int* a, int* b) {
  return *a > *b ? a : b;
}

void swap(int* x, int* y) {
  int tmp = *x;
  *x = *y;
  *y = *x;
}

int main() {
  int* p0 = (int*)malloc(4);
  int* p1 = (int*)malloc(4);
  int* p2 = (int*)malloc(4);
  int* p3 = (int*)malloc(4);
  int* p4;
  int* p5;

  *p0 = 0;
  *p1 = 1;
  *p2 = 2;
  *p3 = 3;

  p4 = maxPointer(p0, p1);
  p5 = maxPointer(p2, p3);
/*  swap(p0, p1);
  swap(p2, p3);
  */

  return *p0 + *p1 + *p2 + *p3;
}
