#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"

int
main(int argc, char* argv[]){

    int i;
    int x;

    if (argc != 3){
        exit();
    }

    struct pstat* procs = (struct pstat*) malloc(sizeof(struct pstat));
    getpinfo(procs);
    settickets((int) atoi(argv[2]));
    for(i = 0; i < atoi(argv[1]); i++){
        x += i;
    }
    exit();
    return x;
}
