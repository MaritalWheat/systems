#include "types.h"
#include "user.h"

int
main(int argc, char *argv[]){
  int *p = 0;// p is on a stack
  printf(1, "%x\n", *p);// try to read the value at address 0;
  exit();
}
