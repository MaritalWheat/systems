#ifndef __RPC_h__
#define __RPC_h__

//
// includes
// 

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <fcntl.h>
#include <assert.h>
#include <stdint.h>

#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>

#include <netinet/tcp.h>
#include <netinet/in.h>
#include "udp.h"

#define MFS_BLOCK_SIZE   (4096)
#define MAX_HOSTNAME_LEN (64)
#define MAX_FILENAME_LEN (60)

int send_message( void * packet, int packet_length);

enum FUNCTIONS {
    INIT = 1,
    LOOKUP = 2,
    STAT = 3,
    WRITE = 4,
    READ = 5,
    CREAT = 6,
    UNLINK = 7,
    SHUTDOWN = 100
};

// this is needed on both server and client, so it is included here
typedef struct __MFS_Stat_t {
    int type;   // MFS_DIRECTORY or MFS_REGULAR
    int size;   // bytes
    int blocks; // number of blocks allocated to file
    // note: no permissions, access times, etc.
} MFS_Stat_t;


/***********************************************************
 * stat call and response
 ***********************************************************/
typedef struct {
    int32_t function;
    int32_t inum;                   // inode number
} stat_call;

typedef struct {
    // only one failure mode, so no field included for error type
    int32_t error;
    MFS_Stat_t stat;                // file statistics structure 
} stat_response;

/***********************************************************
 * read call and response
 ***********************************************************/
typedef struct {
    int32_t function;
    int32_t inum;                   // inode number
    int32_t block;                  // block offset
} read_call;

typedef struct {
    int32_t error;
    char buffer[MFS_BLOCK_SIZE];    // data read
} read_response;


/***********************************************************
 * other calls that accept a generic repsonse
 ***********************************************************/
typedef struct {
    // error code of 0 indicates success, individual calls can define
    // other error codes as necessary
    int32_t error;
    int32_t ret;
} generic_response;

// this might not need to be sent to the server, might just be client init
typedef struct init_call{
    int32_t function;
    int32_t port;
    char hostname[MAX_HOSTNAME_LEN];
} create_call;

typedef struct {
    int32_t function;
    int32_t pinum;                  // parent inode number
    char name[MAX_FILENAME_LEN];    // file name 
} lookup_call;

typedef struct {
    int32_t function;
    int32_t inum;                   // inode number
    int32_t block;                  // block offset
    char buffer[MFS_BLOCK_SIZE];    // data to write 
} write_call;

typedef struct {
    int32_t function;
    int32_t pinum;                  // parent inode number
    int32_t type;                   // file or directory
    char name[MAX_FILENAME_LEN];    // file name 
} creat_call;

typedef struct {
    int32_t function;
    int32_t pinum;                  // parent inode number
    char name[MAX_FILENAME_LEN];    // file name 
} unlink_call;

typedef struct {
    int32_t function;
} shutdown_call;
#endif // __RPC_h__

