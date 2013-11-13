#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include "sort.h"

void usage() 
{
    fprintf(stderr, "Usage: fastsort -i inputfile -o outputfile\n");
    exit(1);
}

int cmpfunc (const void * a, const void * b)
{
   const rec_t *_a = a;
   const rec_t *_b = b;
   
   if(_a->key > _b->key) return   1;
   if(_a->key < _b->key) return  -1;
   return 0;
}

int main(int argc, char *argv[])
{
  char *defFile   = "/no/such/file";
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
              usage();
      }
  }
  
  if (inFile == defFile || outFile == defFile) {
    usage();
  }
  // open and create output file
  int fd = open(inFile, O_RDONLY);
  if (fd < 0) {
      fprintf(stderr, "Error: Cannot open file %s\n", inFile);
      exit(1);
  }
  
  int out = open(outFile, O_WRONLY|O_CREAT|O_TRUNC, S_IRWXU);
  if (out < 0) {
    fprintf(stderr, "Error: Cannot open file %s\n", outFile);
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
	fprintf(stderr, "Error: Cannot open file %s", inFile);
	exit(1);
    }
    
    tmp = realloc(array, rec_counter * sizeof(rec_t));
    if(tmp == NULL) {
      fprintf(stderr, "Error: failed realloc.");
      exit(1);
    } else {
      array = tmp;
    }
      
    array[rec_counter - 1] = r;
    rec_counter += 1;
  }

  qsort(array, rec_counter - 1, sizeof(rec_t), cmpfunc);
  
  int i;
  rec_t _wr;
  for (i = 0; i < rec_counter - 1; i++) {
	_wr.key = array[i].key;
	int j;
	for (j = 0; j < NUMRECS; j++) {
	    _wr.record[j] = array[i].record[j] ;
	}

	int rc = write(out, &_wr, sizeof(rec_t));
	if (rc != sizeof(rec_t)) {
	    fprintf(stderr, "Error: Cannot open file %s", outFile);
	    exit(1);
	    // should probably remove file here but ...
    }
  }
  
  (void) close(fd);
  (void) close(out);
  return 0;
}
