#include "types.h"
#include "user.h"

int
main(int argc, char *argv[]){
  char *p = (char*) ((640 * 1024) - 4096);
  *p = 'a';
  printf(1, "p: %x %c\n", p, *p);
  // this address has not been added to our space
  // printf(1, "%x\n", *p);// try to read the value at address 0;
  
  int rc = fork();
  if (rc == 0 ){
    printf(1, "child: %c\n", *p);
  }
  else if (rc > 0){
    (void) wait();
    printf(1, "parent: %c\n", *p);
  }
  else{
    printf(1, "something horrible happened");
  }
  exit();
}
