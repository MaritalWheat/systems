#include "types.h"
#include "user.h"


int
main(int argc, char *argv[])
{
  printf(1, "TEST SUCCESS: ");
  int tmp = getsyscallinfo();
  printf(1, "%d \n", tmp);
  exit();
}
