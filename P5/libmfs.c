#include "mfs.h"

// largest response message 1 block plus room for error code
#define BUFFER_SIZE (4096 + 4)

// this struct will store server identification info
struct sockaddr_in saddr;
// single buffer for requests, assumes single operations handled at a time
char receive_buffer[BUFFER_SIZE];
int sd;

int MFS_Init(char *hostname, int port){
    sd = UDP_Open(-1);
    assert(sd > -1);
    int rc = UDP_FillSockAddr(&saddr, hostname, port);
    if (rc > 0)
        return 1;
    else
        return 0;
}

/**********************************************************
 * Send a message to the server, need to pass buffer for
 * expected return packet.
 **********************************************************/
int send_message(void * packet, int packet_length){
    struct sockaddr_in raddr;
    fd_set r;
    FD_ZERO(&r);
    FD_SET(sd, &r);

    struct timeval t;
    t.tv_sec = 5;
    t.tv_usec = 0;

    int rc;
    int i;
    // number of times a client will retry a request, server crash should
    // not make client request indefinitely
    int MAX_TRIES = 5;
    for (i = 0; i < MAX_TRIES; i++){
        rc = UDP_Write(sd, &saddr, packet, packet_length);
        if (rc > 0) { // no error with sent mesasge
            t.tv_sec = 3;
            t.tv_usec = 0;
            printf("CLIENT:: function #: %d\n", ((shutdown_call *) packet)->function);
            rc = select(sd + 1, &r, NULL, NULL, &t); 
            if (rc > 0){
                int rc = UDP_Read(sd, &raddr, receive_buffer,
                        BUFFER_SIZE);
                printf("CLIENT:: read %d bytes (message: '%s')\n", rc, receive_buffer);
                return 1;
            }
            else{
                printf("CLIENT: timed out\n");
            }
        }
    }
    return 0;
}

int MFS_Lookup(int pinum, char *name){
	lookup_call req;
	req.function = LOOKUP;
	req.pinum = pinum;
	memcpy(req.name, name, MAX_FILENAME_LEN);
	send_message((void *) &req, sizeof(lookup_call));
    if (((generic_response *) receive_buffer)->error) {
        //printf("MFS_Lookup received error - CLIENT\n");
	return -1;
    } else {
        //printf("MFS_Lookup received return value (%i) - CLIENT\n", ((generic_response *) receive_buffer)->ret);
        return ((generic_response *) receive_buffer)->ret;
    }
}

int MFS_Stat(int inum, MFS_Stat_t *m){
	stat_call req;
	req.function = STAT;
	req.inum = inum;
	send_message((void *) &req, sizeof(stat_call));
    if (((generic_response *) receive_buffer)->error){
        return -1;
    }
    else{
	    memcpy(m, &((stat_response *) receive_buffer)->stat, sizeof(MFS_Stat_t));
        printf("size: %d, type: %d, blocks: %d\n", m->size, m->type, m->blocks); 
        return 0;
    }
}

int MFS_Write(int inum, char *buffer, int block){
	write_call req;
	req.function = WRITE;
	req.inum = inum;
	memcpy(req.buffer, buffer, MFS_BLOCK_SIZE);
    req.block = block;
    fprintf(stderr, "!|!|!|!------|!|!|! error code: \n");

	send_message((void *) &req, sizeof(write_call));
    fprintf(stderr, "!|!|!|!------|!|!|! error code: %d\n", ((generic_response *) receive_buffer)->error);
    if (((generic_response *) receive_buffer)->error == -1)
        return -1;
    else
        return ((generic_response *) receive_buffer)->ret;
}

int MFS_Read(int inum, char *buffer, int block){
	read_call req;
	req.function = READ;
	req.inum = inum;
	req.block = block;
	send_message((void *) &req, sizeof(read_call));
    if (((generic_response *) receive_buffer)->error){
        return -1;
    }
    else{
	    memcpy(buffer, ((read_response *) receive_buffer)->buffer, MFS_BLOCK_SIZE);
        return 0;
    }
}

int MFS_Creat(int pinum, int type, char *name){
	creat_call req;
	req.function = CREAT;
	req.pinum = pinum;
	req.type = type;
	memcpy(req.name, name, MAX_FILENAME_LEN);
	send_message((void *) &req, sizeof(creat_call));
    if (((generic_response *) receive_buffer)->error) {
	fprintf(stderr, "CLIENT::MFS_Creat Received Error\n");
        return -1;
    } else {
	fprintf(stderr, "CLIENT::MFS_Creat Received RC (%i)\n", ((generic_response *) receive_buffer)->ret);
        return ((generic_response *) receive_buffer)->ret;
    }
}

int MFS_Unlink(int pinum, char *name){
	unlink_call req;
	req.function = UNLINK;
	req.pinum = pinum;
	memcpy(req.name, name, MAX_FILENAME_LEN);
	send_message((void *) &req, sizeof(unlink_call));
    if (((generic_response *) receive_buffer)->error)
        return -1;
    else
        return ((generic_response *) receive_buffer)->ret;
}

int MFS_Shutdown(){
	shutdown_call req;
	req.function = SHUTDOWN;
	send_message((void *) &req, sizeof(shutdown_call));
    if (((generic_response *) receive_buffer)->error)
        return -1;
    else
        return ((generic_response *) receive_buffer)->ret;
}
