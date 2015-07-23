#include <stdio.h>
#include "udp.h"
#include "mfs.h"

#define BUFFER_SIZE (4500)
const int BLOCK_SIZE = 4096;
const int INODE_SIZE = 64;
const int IMAP_CHUNK_SIZE = 64;

int sd;
struct sockaddr_in saddr;
char receive_buffer[BUFFER_SIZE];

const int IMAP_LOC = 4;
const int LOG_START = 4096;

const int t_FILE = 1;
const int t_DIR = 0;


checkpoint_reg cp_reg;
int inode_map[4096];
int fs_handle;
int curr_map_chunk = 0;
int curr_chunk_location = 0;
int curr_inum = 1;

void UpdateCheckpointRegion(int fs_handle)
{
    int rc;
    lseek(fs_handle, 0, SEEK_SET);
    rc = write (fs_handle, (char*)&cp_reg, BLOCK_SIZE);
    lseek(fs_handle, cp_reg.log_end, SEEK_SET);
    fsync(fs_handle);
}

void UpdateLogEnd(int fs_handle, int to_add)
{
    cp_reg.log_end += to_add;
    UpdateCheckpointRegion(fs_handle);
}

void UpdateImapTable(int fs_handle, int chunk_location) 
{
    cp_reg.map_chunks[curr_map_chunk] = chunk_location;
    UpdateCheckpointRegion(fs_handle);
}

//returns start location of parent directory
int NavigateToPinum(int pinum) 
{   
    int chunk_to_look_in = pinum / 16;
    int location_in_chunk = pinum % 16;
    
    if (chunk_to_look_in > curr_map_chunk || location_in_chunk > curr_chunk_location) return -1;

    int seek = lseek(fs_handle, IMAP_LOC + sizeof(int) * chunk_to_look_in, SEEK_SET);
    int chunk_loc[1];
    int rc = read(fs_handle, chunk_loc, sizeof(int));
    
    printf("Location of chunk holding pinum: %i\n", (int)chunk_loc[0]);
    
    int pinode_loc[1];
    seek = lseek(fs_handle, (int)chunk_loc[0] + sizeof(int) * location_in_chunk, SEEK_SET);
    rc = read(fs_handle, pinode_loc, sizeof(int));
    
    printf("Location of pinode_loc: %i\n", (int)pinode_loc[0]);

    int pdata_loc[1];
    seek = lseek(fs_handle, (int)pinode_loc[0] + 8, SEEK_SET);
    rc = read(fs_handle, pdata_loc, sizeof(int));
    
    printf("Location of pdata_loc: %i\n", (int)pdata_loc[0]);
    
    return (int)pdata_loc[0];
}

int Lookup (int pinum, char * name)
{
    fprintf(stderr, "Server::Lookup(%i, %s)\n", pinum, name);
    if (strlen(name) > 60) return -1;
    int seek;
    int rc;
    //search through directory entries
    int dir_start_loc = NavigateToPinum(pinum);
    if (dir_start_loc == -1) return -1; //error
    int entries_searched = 0;
    int valid = 0;
    
    //value to hold inum of name searched for
    int inum = -1;
    
    while (valid != -1 && entries_searched < 64) {
      dir_entry pdir_entries[1];
      seek = lseek(fs_handle, dir_start_loc, SEEK_SET);
      rc = read(fs_handle, pdir_entries, sizeof(dir_entry));
      
      fprintf(stderr, "	Name: %s, Inum: %i\n", pdir_entries[0].name, pdir_entries[0].inum);
      //handle match found
      if (strcmp(name, pdir_entries[0].name) == 0) {
	inum = pdir_entries[0].inum;
      }
      
      valid = pdir_entries[0].inum;   
      entries_searched++;
      dir_start_loc += sizeof(dir_entry);
      
      if (valid == -1 || entries_searched == 64) {
	//handle name not found
	break;
      }
      
      printf("Directory entry: <%s, %i>\n", pdir_entries[0].name, pdir_entries[0].inum);
   
    }
    
    return inum;
}

void AppendData(char * data, int size) 
{
    lseek(fs_handle, cp_reg.log_end, SEEK_SET);
    write(fs_handle, data, size);
    UpdateLogEnd(fs_handle, size);
}

int GetChunkLocation(int inum)
{
    int rc, seek;
    int chunk_to_look_in = inum / 16;
    fprintf(stderr, "get chunk location\n");
    if (chunk_to_look_in > curr_map_chunk){
        fprintf(stderr,"outside of chunk\n");
        return -1;
    }
    
    seek = lseek(fs_handle, IMAP_LOC + sizeof(int) * chunk_to_look_in, SEEK_SET);
    int chunk_loc[1];
    rc = read(fs_handle, chunk_loc, sizeof(int));
    
    fprintf(stderr, "Location of chunk holding pinum: %i\n", (int)chunk_loc[0]);
    
    return (int)chunk_loc[0];
}


int GetInodeLocation(int chunk_loc, int index_in_chunk) 
{
    int seek, rc;
    int pinode_loc[1];
    seek = lseek(fs_handle, chunk_loc + sizeof(int) * index_in_chunk, SEEK_SET);
    rc = read(fs_handle, pinode_loc, sizeof(int));
    
    //printf("Location of inode: %i\n", (int)pinode_loc[0]);
    return (int)pinode_loc[0];
}

int GetDataLocation(int inode_loc, int index_in_inode) 
{
    int rc, seek;
    int pdata_loc[1];
    seek = lseek(fs_handle, inode_loc + 8 + sizeof(int) * index_in_inode, SEEK_SET); //+8 is offset within inode to get to direct pointers (2 ints)
    rc = read(fs_handle, pdata_loc, sizeof(int));
    
    //printf("Location of data block: %i\n", (int)pdata_loc[0]);
    return (int)pdata_loc[0];
}

dir_block GetDirData(int data_loc) {
    int rc, seek;
    dir_block dir_data[1];
    seek = lseek(fs_handle, data_loc, SEEK_SET); //+8 is offset within inode to get to direct pointers (2 ints)
    rc = read(fs_handle, dir_data, sizeof(dir_block));
    
    //printf("Location of data block: %i\n", (int)pdata_loc[0]);
    return dir_data[0];
}

inode GetInodeData(int inode_loc)
{
    int rc, seek;
    inode inode_data[1];
    seek = lseek(fs_handle, inode_loc, SEEK_SET); 
    rc = read(fs_handle, inode_data, sizeof(inode));
    
    //printf("Location of data block: %i\n", (int)pdata_loc[0]);
    return inode_data[0];
}

inode GetInode(int inum){
    return GetInodeData(GetInodeLocation(GetChunkLocation(inum), inum % 16));
}

map_chunk GetMapChunkData(int chunk_loc)
{
    int rc, seek;
    map_chunk chunk_data[1];
    seek = lseek(fs_handle, chunk_loc, SEEK_SET); 
    rc = read(fs_handle, chunk_data, sizeof(map_chunk));
    
    //printf("Location of data block: %i\n", (int)pdata_loc[0]);
    return chunk_data[0];
}

int CheckFileType(int inode_loc) 
{
    int rc, seek;
    int file_type[1];
    seek = lseek(fs_handle, inode_loc + 4, SEEK_SET); 
    rc = read(fs_handle, file_type, sizeof(int));
    
    printf("File type: %i\n", (int)file_type[0]);
    return (int)file_type[0];
}

void AddDirectoryEntry(int dir_start_loc, char * name) 
{
    int seek;
    int rc;
    
    //search through directory entries
    int entries_searched = 0;
    int valid = 0;
    printf("Add directory entry called.\n");
    while (valid != -1 && entries_searched < 64) {
      dir_entry pdir_entries[1];
      seek = lseek(fs_handle, dir_start_loc, SEEK_SET);
      rc = read(fs_handle, pdir_entries, sizeof(dir_entry));
      
      valid = pdir_entries[0].inum;   
      entries_searched++;
      
      if (valid == -1) {
	dir_entry insertable;
	insertable.inum = curr_inum;
	curr_inum++;
	strcpy(insertable.name, name);
	
	seek = lseek(fs_handle, dir_start_loc, SEEK_SET);
	write(fs_handle, (char*)&insertable, sizeof(dir_entry));
	
	/*dir_start_loc += sizeof(dir_entry);
	dir_entry tmp[1];
	seek = lseek(fs_handle, dir_start_loc, SEEK_SET);
	rc = read(fs_handle, tmp, sizeof(dir_entry));
	printf("Inserted: <%s, %i>\n", tmp[0].name, tmp[0].inum);*/
	break;
      }
      
      dir_start_loc += sizeof(dir_entry);
      printf("Directory entry: <%s, %i>\n", pdir_entries[0].name, pdir_entries[0].inum);
   
    }
}

//update the inode to point to new data location 
void UpdateInodeMapping(int inode_loc, int data_loc, int index_in_inode) {
    int seek, rc;
    int new_loc = cp_reg.log_end - BLOCK_SIZE;
    seek = lseek(fs_handle, inode_loc + 8 + sizeof(int) * index_in_inode, SEEK_SET); //+8 is offset within inode to get to direct pointers (2 ints)
    rc = write(fs_handle, (char*)&new_loc, sizeof(int));
}

//update imap chunk to point to new inode location
void UpdateImapChunk(int chunk_loc, int index_in_chunk)
{
    int seek, rc;
    int new_loc = cp_reg.log_end - INODE_SIZE;
    fprintf(stderr, "before seek\n");
    seek = lseek(fs_handle, chunk_loc + sizeof(int) * index_in_chunk, SEEK_SET);
    fprintf(stderr,"after seek\n");
    rc = write(fs_handle, (char*)&new_loc, sizeof(int));
    fprintf(stderr, "after write\n");
}

int Create(int pinum, int type, char * name) 
{   
    if (strlen(name) > 60) return -1;
    int pinum_loc = NavigateToPinum(pinum);
    int chunk_loc = GetChunkLocation(pinum);
    if (chunk_loc == -1) return -1; //error
    
    int i, j;
    int pinum_found = -1;
    int imap_index = -1;
    
    for (i = 0; i < 16; i++) {
      int inode_loc = GetInodeLocation(chunk_loc, i);

      if (inode_loc != 0) { //only check valid inode locations
	int file_type = CheckFileType(inode_loc);
	for (j = 0; j < 14; j++) {
	  int data_loc = GetDataLocation(inode_loc, j);

	  if (data_loc == 0) break;//only copy valid data block
	  
	  //handle directory files
	  if (file_type == t_DIR) {
	    if (data_loc == pinum_loc) {
	      AddDirectoryEntry(data_loc, name);
	      pinum_found = 0;
	    }
	    dir_block data = GetDirData(data_loc);
	    
	    fprintf(stderr, "	Directory block data: %s\n", (char*)&data.entries[2]);
	    
	    AppendData((char*)&data, BLOCK_SIZE);
	    if (pinum_found == 0) {
	      UpdateInodeMapping(inode_loc, data_loc, j);
	      pinum_found = -1;
	      imap_index = i;
	    }
	  } else if (file_type == t_FILE) {
	     if (data_loc == pinum_loc) {
	      return -1;
	    }
	  }
	  
	}
	inode inode_data = GetInodeData(inode_loc);
	
	//printf("inode data size is: %i\n", inode_data.size);
	AppendData((char*)&inode_data, INODE_SIZE);
	UpdateImapChunk(chunk_loc, i);
      }
    }
    if (type == t_DIR) {
	dir_block new_directory;
	dir_entry first, second;
    
	//first.name = ".";
	strcpy(first.name, ".");
	first.inum = 1;
	
	//second.name = "..";
	strcpy(second.name, "..");
	second.inum = pinum;
	
	new_directory.entries[0] = first;
	new_directory.entries[1] = second;
	
	int i;
	for (i = 2; i < 64; i++) {
	  dir_entry tmp;
	  tmp.inum = -1;
	  strcpy(tmp.name, "");
	  new_directory.entries[i] = tmp;
	}
	
	AppendData((char*)&new_directory, BLOCK_SIZE);
    }
    
    //append new inode
    inode new_inode;
    new_inode.size = 0;
    new_inode.type = type;
    if (type == t_DIR) {
      new_inode.block_locations[0] = cp_reg.log_end - BLOCK_SIZE;
    }
    
    //write new inode location to imap chunk
    AppendData((char*)&new_inode, INODE_SIZE);
    UpdateImapChunk(chunk_loc, imap_index + 1);
    
    //append new imap chunk
    map_chunk imap_chunk = GetMapChunkData(chunk_loc);
    AppendData((char*)&imap_chunk, IMAP_CHUNK_SIZE);
   
    //update checkpoint region
    UpdateImapTable(fs_handle, cp_reg.log_end - IMAP_CHUNK_SIZE);

    return 0;
}

int Init(char * filename)
{
    fs_handle = open (filename, O_RDWR | O_CREAT, 0777); //might need different permissions?
    if (fs_handle < 0) return -1;
    
    //write end-of-log location
    cp_reg.log_end = 0 + BLOCK_SIZE; //set current end-of-log location to the end of the checkpoint region
    UpdateCheckpointRegion(fs_handle);
    
    //write out root directory entries 
    dir_block root_data;
    dir_entry first, second;
    
    //first.name = ".";
    strcpy(first.name, ".");
    first.inum = 0;
    
    //second.name = "..";
    strcpy(second.name, "..");
    second.inum = 0;
    
    root_data.entries[0] = first;
    root_data.entries[1] = second;
    int i;
    for (i = 2; i < 64; i++) {
      dir_entry tmp;
      tmp.inum = -1;
      root_data.entries[i] = tmp;
    }
    
    write(fs_handle, (char*)&root_data, BLOCK_SIZE);
   
    UpdateLogEnd(fs_handle, BLOCK_SIZE);
    
    //write out root directory inode
    inode root_inode;
    root_inode.size = BLOCK_SIZE;
    root_inode.type = t_DIR;
    root_inode.block_locations[0] = cp_reg.log_end - BLOCK_SIZE;
    
    write(fs_handle, (char*)&root_inode, INODE_SIZE);
    
    UpdateLogEnd(fs_handle, INODE_SIZE);
    
    //write out imap chunk
    map_chunk block_chunk;
    printf("Inode Location (in chunk): %i\n", cp_reg.log_end - INODE_SIZE);
    block_chunk.inode_locations[0] = cp_reg.log_end - INODE_SIZE;
    curr_chunk_location++;
    
    write(fs_handle, (char*)&block_chunk, IMAP_CHUNK_SIZE);
    
    UpdateLogEnd(fs_handle, IMAP_CHUNK_SIZE); 
    
    //update imap table with chunk location
    printf("Chunk Location: %i\n", cp_reg.log_end - IMAP_CHUNK_SIZE);
    UpdateImapTable(fs_handle, cp_reg.log_end - IMAP_CHUNK_SIZE);
    
    return 0;
}

/**********************************************************
 * Send a message to the server, need to pass buffer for
 * expected return packet.
 **********************************************************/
int send_message(void * packet, int packet_length){
    printf("SERVER:: send response");
    fd_set r;
    FD_ZERO(&r);
    FD_SET(sd, &r);

    struct timeval t;
    t.tv_sec = 5;
    t.tv_usec = 0;

    int rc;

    rc = UDP_Write(sd, &saddr, packet, packet_length);
    if (rc > 0) { // no error with sent mesasge
        return 1;
    }
    else{
        return 0;
    }
}

//====================================================
//ALL METHODS ARE NAMED THE SAME AS THE CLIENT LIBRARY
//====================================================

/***********************************************************
 * Many of the method just return an error code an a single
 * value, this method does this for all such methods.
 ***********************************************************/
int send_generic_response(int error, int value){
	generic_response ret;
	ret.error = 0;
    ret.ret = value;
	return send_message((void *) &ret, sizeof(generic_response));
}

int MFS_Lookup(int pinum, char *name){
    fprintf(stderr, "Server::MFS_Lookup\n");
    // lookup inum, place in int below
    int rc = Lookup(pinum, name);
      fprintf(stderr, "Server::MFS_Lookup RC: %i\n", rc);
    int error = 0;
    int inum = rc;
    send_generic_response(error, inum);
    return 0;
}

int MFS_Stat(int inum, MFS_Stat_t *m){
    // read file statistics from FS, place in pointer to state structure passed

    inode node = GetInode(inum);
    fprintf(stderr, "SERVER:: stat - inode [%d, %d, (%d, %d, %d)]\n", node.type, node.size, 
            node.block_locations[0],
            node.block_locations[1],
            node.block_locations[2]);
    // here is code for populating and sending response 
	stat_response ret;
    m->type = node.type;
    m->size = node.size;
    int i = 0;
    for (i = 0; i < 14; i++){
        if (node.block_locations[i] == 0)
            break; 
    }
    m->blocks = i;
	ret.error = 0;
	memcpy(&ret.stat, m, sizeof(MFS_Stat_t));
	send_message((void *) &ret, sizeof(stat_response));
    return 0;
}

int MFS_Write(int inum, char *buffer, int block){
    int error = 0;
    fprintf(stderr, "@@@@@@@@@@----------- %d\n", block);
    if (block > 13 || block < 0){
        error = -1;
    } else {
        fprintf(stderr, "SERVER:: write called\n");
        fprintf(stderr, "write this: %s\n", buffer);
        fprintf(stderr,"!-----!!!!@# end of write: %s\n",(char *) buffer + MFS_BLOCK_SIZE - 5);
        inode node = GetInode(inum);
        fprintf(stderr, "block: %d inode [%d, %d, (%d, %d, %d)]\n", block, node.type, node.size, 
                node.block_locations[0],
                node.block_locations[1],
                node.block_locations[2]);
        int end = cp_reg.log_end;
        AppendData(buffer, MFS_BLOCK_SIZE);
        fprintf(stderr, "-----after append\n");
        node.block_locations[block] = end;
        fprintf(stderr, "-----after append\n");
        int chunk_loc = GetChunkLocation(inum);
        fprintf(stderr, "after append, log end: %d \n", cp_reg.log_end);
        fprintf(stderr, "after chunk location and update imap chunk\n");
        node.size = MFS_BLOCK_SIZE * (block + 1);
        AppendData((char*)&node, sizeof(inode));
        UpdateImapChunk(chunk_loc, inum % 16);
        fprintf(stderr, "after append\n");
    }

    // no return value
    fprintf(stderr, "@@@@@@@@@@----------- %d\n", error);
    send_generic_response(error,error);
    return 0;
}

int MFS_Read(int inum, char *buffer, int block){
    // do actual read from FS
    int error = 0;
    read_response ret;
    fprintf(stderr, "@@@@@@@@@@----------- %d\n", block);
    if (block > 13 || block < 0){
        error = -1;
        //ret.error = error;
    } else {
        inode node = GetInode(inum);
        fprintf(stderr, "block: %d inode [%d, %d, (%d, %d, %d)]\n", block, node.type, node.size, 
                node.block_locations[0],
                node.block_locations[1],
                node.block_locations[2]);
        dir_block data = GetDirData(node.block_locations[block]);      
        fprintf(stderr,"      after read: %s\n",(char *) &data);
        fprintf(stderr,"      after read: %s\n",(char *) &(data) + MFS_BLOCK_SIZE - 5);
        // here is code for populating and sending response
        memcpy(ret.buffer, &data, MFS_BLOCK_SIZE);
    }
    ret.error = error;
    fprintf(stderr, "@@@@@@@@@@ error----------- %d\n", error);
	send_message((void *) &ret, sizeof(read_response));
    return 0;
}

int MFS_Creat(int pinum, int type, char *name){
    fprintf(stderr, "Server::MFS_Creat\n");
    int rc = Create(pinum, type, name);
    fprintf(stderr, "Server::MFS_Creat Error: %i\n", rc);

    // no return value
    send_generic_response(0, rc);
    return 0;
}

int MFS_Unlink(int pinum, char *name){
    int rc = 0;
    if (strlen(name) > 60) return rc;
    int error = rc;
    // no return value
    send_generic_response(error, 0);
    return 0;
}

int MFS_Shutdown(){
    // shutdown the server
    // nothing to return to client
    // send a message to let them know they do not need to resubmit
    int error = 0;
    // no return value
    send_generic_response(error, 0);
	exit(0);
}

int
main(int argc, char *argv[])
{
    // open port, make sure it is avaialable  
    sd = UDP_Open(atoi(argv[1]));
    //assert(sd > -1);
    Init(argv[2]);
    //int inum = Lookup(0, "..");
    //Create(0, t_FILE, "..");
    //printf("Returning inum: %i\n", inum);
     
    printf("   SERVER:: waiting in loop\n");
    
    // do sever stuff separate from network peice, run methods in local main

    while (1) {
        char buffer[BUFFER_SIZE];
        // can also send a struct nd sizeof(struct)
        int rc = UDP_Read(sd, &saddr, buffer, BUFFER_SIZE);
        if (rc > 0) {
            lookup_call * lookup;
            stat_call * stat;
            write_call * write;
            read_call * read;
            creat_call * creat;
            unlink_call * unlink;
            shutdown_call * shutdown;
            MFS_Stat_t file_info;
                    int i;
            // stra four bits are to return status before the block
            char read_buffer[MFS_BLOCK_SIZE + 4]; 
			switch (*((int *) buffer)){

				case LOOKUP:
                    lookup = (lookup_call *) buffer;
					printf("SERVER- lookup(pinum:%d, name:%s)\n", lookup->pinum, 
                            lookup->name);
                    MFS_Lookup(lookup->pinum, lookup->name);
					break;
				case STAT:
                    stat = (stat_call *) buffer;
                    printf("SERVER- stat(inum:%d)\n",stat->inum);
                    MFS_Stat(stat->inum, &file_info);
					break;
				case WRITE:
                    write = (write_call *) buffer;
                    fprintf(stderr, "SERVER- write(inum:%d, block:%d, file_contents:%s)\n", 
                            write->inum, write->block, write->buffer);
                    fprintf(stderr,"RPC CONTENTS::  write (last few bytes): %s\n",(char *) write->buffer + MFS_BLOCK_SIZE - 5);
                    for (i= 0; i < MFS_BLOCK_SIZE; i++){
                        fprintf(stderr,"%c", write->buffer[i]);
                    }
                    MFS_Write(write->inum, write->buffer, write->block);
					break;
				case READ:
                    read = (read_call *) buffer;
                    printf("SERVER- read( inum: %d, block: %d)\n",read->inum, 
                            read->block);
                    MFS_Read(read->inum, read_buffer, read->block);
					break;
				case CREAT:
                    creat = (creat_call *) buffer;
                    printf("SERVER- creat(pinum:%d, type:%d, name:%s)\n", 
                            creat->pinum, creat->type, creat->name);
                    MFS_Creat(creat->pinum, creat->type, creat->name);
					break;
				case UNLINK:
                    unlink = (unlink_call *) buffer;
                    printf("SERVER- unlink(pinum:%d, name:%s)\n", unlink->pinum,
                            unlink->name);
                    MFS_Unlink( unlink->pinum, unlink->name);
					break;
				case SHUTDOWN:
                    shutdown = (shutdown_call *) buffer;
                    printf("SERVER- shutdown()\n");
                    MFS_Shutdown();
					break;
			}
            //char reply[BUFFER_SIZE];
            //sprintf(reply, "reply");
            //rc = UDP_Write(sd, &saddr, reply, BUFFER_SIZE);
        }
    }

    return 0;
}


