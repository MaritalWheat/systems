#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include "sort.h"

void usage(char *prog) 
{
    fprintf(stderr, "usage: %s <-s random seed> <-n number of records> <-o output file>\n", prog);
    exit(1);
}

int main(int argc, char *argv[])
{
  char *inFile    = "/no/such/file";
  char *outFile   = "/no/such/file";
 
  int c;
  opterr = 0;
  while ((c = getopt(argc, argv, "i:o:")) != -1) {
      switch (c) {
      case 'i':
	  inFile      = strdup(optarg);
	  break;
      case 'o':
	  outFile     = strdup(optarg);
	  break;
      default:
	  usage(argv[0]);
      }
  }
  
  // open and create output file
  int fd = open(inFile, O_RDONLY);
  if (fd < 0) {
      perror("open");
      exit(1);
  }
  
  int out = open(outFile, O_WRONLY|O_CREAT|O_TRUNC, S_IRWXU);
  if (out < 0) {
    perror("open");
    exit(1);
  }

  rec_t r;
  rec_t *array;
  rec_t *tmp;
  array = malloc(sizeof(rec_t));
  int rec_counter = 1;
  
  
  while (1) {	
    int rc;
    
    rc = read(fd, &r, sizeof(rec_t)); 
    
    if (rc == 0) // 0 indicates EOF
	break;
    if (rc < 0) {
	perror("read");
	exit(1);
    }
    
    //printf("Resize: %u --", (int)(rec_counter * sizeof(rec_t)));
    tmp = realloc(array, rec_counter * sizeof(rec_t));
    if(tmp == NULL) {
      printf("FAILED REALLOC");
    } else {
      array = tmp;
    }
    array[rec_counter - 1] = r;
    
    
    printf("key: %u    | rec: ", array[(rec_counter - 1)].key);
    int k;
	for (k = 0; k < NUMRECS; k++) 
	    printf("%u ", array[(rec_counter - 1)].record[k]);
	printf("\n");
    rec_counter += 1;
    
    int j;
    for (j = 0; j < NUMRECS; j++) {
      
      rc = write(out, &r, sizeof(rec_t));
      if (rc != sizeof(rec_t)) {
	  perror("write");
	  exit(1);
	  // should probably remove file here but ...
      }
    }
  }

  (void) close(fd);
  (void) close(out);
  return 0;
}
