
#ifndef __MFS_h__
#define __MFS_h__

// inlcuded on server and client, defines format for messages

#include "rpc.h"

#define MFS_DIRECTORY    (0)
#define MFS_REGULAR_FILE (1)

typedef struct __MFS_DirEnt_t {
    int  inum;      // inode number of entry (-1 means entry not used)
    char name[252]; // up to 252 bytes of name in directory (including \0)
} MFS_DirEnt_t;

///////////////////////////////////////////////////////////////////////////////
//checkpoint region consisting of pointer to end of log and map chunk locations
//////////////////////////////////////////////////////////////////////////////

typedef struct checkpoint_reg {
    int log_end;
    int map_chunks[256];
} checkpoint_reg;

///////////////////////////////////////////////////////////////////////////////
//inode definition: size of file (bytes), type of file (dir/file), 14 direct pointers
//////////////////////////////////////////////////////////////////////////////
typedef struct {
    int size;
    int type;
    int block_locations[14];
} inode;

///////////////////////////////////////////////////////////////////////////////
//directory entries: 60 byte name, 4 byte (int) inode number
//////////////////////////////////////////////////////////////////////////////
typedef struct dir_entry {
    char name[60];
    int inum;
} dir_entry;

///////////////////////////////////////////////////////////////////////////////
//directory block: 64 directory entries
//////////////////////////////////////////////////////////////////////////////
typedef struct dir_block {
    dir_entry entries[64];
} dir_block;

///////////////////////////////////////////////////////////////////////////////
//data block: 4kb of space for file data to be written to
//////////////////////////////////////////////////////////////////////////////
typedef struct data_block {
    char data[4096];
} data_block;

///////////////////////////////////////////////////////////////////////////////
//map chunk: pointers to inode locations
//////////////////////////////////////////////////////////////////////////////
typedef struct map_chunk {
    int inode_locations[16];
} map_chunk;

int MFS_Init(char *hostname, int port);
int MFS_Lookup(int pinum, char *name);
int MFS_Stat(int inum, MFS_Stat_t *m);
int MFS_Write(int inum, char *buffer, int block);
int MFS_Read(int inum, char *buffer, int block);
int MFS_Creat(int pinum, int type, char *name);
int MFS_Unlink(int pinum, char *name);
int MFS_Shutdown();

//open("/dir1/dir2/file.txt") {
//    inodeNumOfDir1 = Lookup(rootDirInumber, "dir1");

#endif // __MFS_h__
