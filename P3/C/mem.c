#include "mem.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>



#define LOCATION_SORTED (0)
#define LENGTH_SORTED (1)

struct TreeNode {
    int             ID;
    int             data;

    void            *mem_head; //always want to be able to point to base of memory chunk
    struct TreeNode *left_child;
    struct TreeNode *right_child;
};

typedef struct TreeNode node;

int                 curr_ID = 0;
int                 m_error;
int                 init_called = 0;
void                *start_of_region;
node                *root_loc;
node                *root_len;


void insertNode(node **toInsert, node **n) {
    if(*n == NULL) {
        *n = *toInsert;
        (*n)->ID = curr_ID;
    } else {
        if((*n)->ID == (*toInsert)->ID)
        {
            printf("\nThis value already exists in the tree!");
        }
        else
        {
            if((*toInsert)->data > (*n)->data)
                insertNode(&(*toInsert), &(*n)->right_child);
            else
                insertNode(&(*toInsert), &(*n)->left_child);
        }
    }
    curr_ID++;
}

void insertNodeLoc(node **toInsert, node **n) {
    if(*n == NULL) {
        *n = *toInsert;
        (*n)->ID = curr_ID;
    } else {
        if((*n)->mem_head == (*toInsert)->mem_head)
        {
            printf("\nThis value already exists in the tree!");
        }
        else
        {
            if((*toInsert)->mem_head > (*n)->mem_head)
                insertNode(&(*toInsert), &(*n)->right_child);
            else
                insertNode(&(*toInsert), &(*n)->left_child);
        }
    }
    curr_ID++;
}

//check for null return value when calling!!
node* searchNode(void* address, node **n) {
    if((*n)->mem_head == address) {
        return (*n);
    }
    //this is pretty dangerous, never checking if value passed in is null...
    if ((*n)->left_child != NULL) {
        searchNode(address, &((*n)->left_child));
    } else if ((*n)->right_child != NULL) {
        searchNode(address, &((*n)->right_child));
    }
}


node* findBestFit(int sizeToFit, node **n) {
    
    if ((*n) == NULL) return;
    //printf("Searching node with size: %i\n", (*n)->data);
    
    if ((*n)->data == sizeToFit) return (*n);
    
    if ((*n)->data > sizeToFit) {
        if ((*n)->left_child != NULL) {
            if ((*n)->left_child->data >= sizeToFit) {
                findBestFit(sizeToFit, &((*n)->left_child));
            } else {
                return (*n);
            }
        } else {
            return (*n);
        }
    } else {
        if ((*n)->right_child != NULL) {
            findBestFit(sizeToFit, &((*n)->right_child));
        }
    }
    //return (*n);
}

node* findWorstFit(int sizeToFit, node **n) {
    
    if ((*n) == NULL) return;
    //printf("Searching node with size: %i\n", (*n)->data);
    
    if ((*n)->data > sizeToFit) {
        if ((*n)->right_child != NULL) {
            findWorstFit(sizeToFit, &((*n)->right_child));
        } else {
            return (*n);
        }
    } else if ((*n)->data == sizeToFit) {
        return (*n);
    } else {
        if ((*n)->right_child != NULL) {
            findWorstFit(sizeToFit, &((*n)->right_child));
        } else {
            //error case
        }
    }
}

node* deleteNode(int ID, node **n, int data) {
    if(*n == NULL)
        printf("\nValue does not exist in tree!\n");
    else
        if((*n)->ID == ID) {
            node *deleted;
            if((*n)->left_child == NULL)
                (*n) = (*n)->right_child;
            else
                if((*n)->right_child == NULL)
                    (*n) = (*n)->left_child;
                else {
                    node *temp = (*n)->right_child;
                    while(temp->left_child != NULL)
                        temp = temp->left_child;
                    (*n) = temp;
                }
            if (deleted == root_len) root_len = NULL;
            if (deleted == root_loc) root_loc = NULL;
            return deleted;
        }
        else
            if(data > (*n)->data)
                deleteNode(ID, &(*n)->right_child, data);
            else
                deleteNode(ID, &(*n)->left_child, data);
}

node* deleteNodeLoc(void *address, node **n) {
    if(*n == NULL)
        printf("\nValue does not exist in tree!\n");
    else
        if((*n)->mem_head == address) {
            node *deleted;
            if((*n)->left_child == NULL)
                (*n) = (*n)->right_child;
            else
                if((*n)->right_child == NULL)
                    (*n) = (*n)->left_child;
                else {
                    node *temp = (*n)->right_child;
                    while(temp->left_child != NULL)
                        temp = temp->left_child;
                    (*n) = temp;
                }
            if (deleted == root_len) root_len = NULL;
            if (deleted == root_loc) root_loc = NULL;
            return deleted;
        }
        else
            if(address > (*n)->mem_head)
                deleteNodeLoc(address, &((*n)->right_child));
            else
                deleteNodeLoc(address, &((*n)->left_child));
}

void displayInOrder(node **n) {
    if((*n) != NULL) {
        displayInOrder(&((*n)->left_child));
        printf("%p | size: %d\n", (*n)->mem_head, (*n)->data);
        displayInOrder(&((*n)->right_child));
    }
}

int Mem_Init(int size_of_region){
    // open the /dev/zero device
    if ( size_of_region < 1 || init_called ){
        m_error = E_BAD_ARGS;
        return -1;
    }
    int fd = open("/dev/zero", O_RDWR);

    int page_size = getpagesize();
    size_of_region = ( size_of_region % page_size ) == 0 ? size_of_region : 
        (size_of_region / page_size + 1) * page_size;
    printf("Rounded size_of_region: %d\n", size_of_region);
    // size_of_region (in bytes) needs to be evenly divisible by the page size
    start_of_region = mmap(NULL, size_of_region, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (start_of_region  == MAP_FAILED) { 
        m_error = E_BAD_ARGS;
        return -1;
    }
    
    //create header
    root_len = start_of_region;
    printf("root_len: %p\n", root_len);
    root_len->data = size_of_region;
    root_len->mem_head = root_len;
    root_len->ID = curr_ID;
    root_loc = start_of_region + sizeof(node);
    root_loc->mem_head = root_len;
    root_loc->ID = curr_ID;
    printf("root_loc: %p\n", root_loc);
    curr_ID++;

    // close the device (don't worry, mapping should be unaffected)
    close(fd);
    init_called = 1;
    return 0;
}

void *Mem_Alloc(int size, int style){
    printf("ALLOC CALLED WITH SIZE: %i\n\n", (int)(size + 2 * sizeof(node)));
    node* to_remove;
    node* to_remove_loc;
    node* new_free;
    node* new_free_loc;
    node* deleted;
    int total_size_free_space;
	size = (int)(size + 2 * sizeof(node)); //each memory locations needs a len & loc sorted node
    
    // used for temporarily allowing arbitrary number of bytes to be added to ptr
    char* temp_ptr;
    switch (style){
        case BESTFIT:
            
            to_remove = findBestFit(size, &root_len);
            //printf("To Remove's Address is: %p\n\n", to_remove->mem_head);
            to_remove_loc = searchNode(to_remove->mem_head, &root_loc);
            if (to_remove_loc != NULL) {
                //printf("To Delete_Loc's Address is: %p\n\n", to_remove_loc->mem_head);
                deleteNodeLoc(to_remove_loc->mem_head, &root_loc);
            }
            deleted = deleteNode(to_remove->ID, &root_len, to_remove->data);
            //printf("deleted: %p\n", deleted);
            total_size_free_space = to_remove->data;
            
            to_remove->data = size;
            // could not find an exact match for size requested
			// TODO - not quite sure how to handle a request for a piece of memory
			// that leaves less than the header size at the end, no way to keep track
			// of it
            if ( total_size_free_space - size > 0){
                // move the pointer to the beginning of the new free space
                temp_ptr = (char*) to_remove;
                temp_ptr += size; 
                new_free = (node*) temp_ptr;
                new_free->data = total_size_free_space - size;
                new_free->mem_head = new_free;
                new_free_loc = new_free->mem_head + sizeof(node);
                new_free_loc->data = 0;
                new_free_loc->mem_head = new_free->mem_head;
				insertNode(&new_free, &root_len);
                insertNodeLoc(&new_free_loc, &root_loc);
            }
            
            return to_remove;
            
            break;
        case WORSTFIT:
            
            to_remove = findWorstFit(size, &root_len);
            //printf("To Remove's Address is: %p\n\n", to_remove->mem_head);
            to_remove_loc = searchNode(to_remove->mem_head, &root_loc);
            if (to_remove_loc != NULL) {
                //printf("To Delete_Loc's Address is: %p\n\n", to_remove_loc->mem_head);
                deleteNodeLoc(to_remove_loc->mem_head, &root_loc);
            }
            deleted = deleteNode(to_remove->ID, &root_len, to_remove->data);
            //printf("deleted: %p\n", deleted);
            total_size_free_space = to_remove->data;
            
            to_remove->data = size;
            // could not find an exact match for size requested
			// TODO - not quite sure how to handle a request for a piece of memory
			// that leaves less than the header size at the end, no way to keep track
			// of it
            if ( total_size_free_space - size > 0){
                // move the pointer to the beginning of the new free space
                temp_ptr = (char*) to_remove;
                temp_ptr += size;
                new_free = (node*) temp_ptr;
                new_free->data = total_size_free_space - size;
                new_free->mem_head = new_free;
                new_free_loc = new_free->mem_head + sizeof(node);
                new_free_loc->data = 0;
                new_free_loc->mem_head = new_free->mem_head;
				insertNode(&new_free, &root_len);
                insertNodeLoc(&new_free_loc, &root_loc);
            }
            
            return to_remove;
            
            break;
        case FIRSTFIT:

            break;
    }
}


int Mem_Free(void *ptr){
    printf("FREE CALLED WITH ADDRESS: %p\n\n", ptr);
    node* tmpPtr = (node*) ptr;
    insertNode(&tmpPtr, &root_len);
    ptr += sizeof(node);
    node* tmpPtr2 = (node*) ptr;
    insertNodeLoc(&tmpPtr2, &root_loc);
}

void Mem_Dump(){

}

void Tree_Dump() {
    printf("PRINT BY LENGTH: \n");
    displayInOrder(&root_len);
    printf("\nPRINT BY LOC: \n");
    displayInOrder(&root_loc);
    printf("\n");
}

int main(){

    Mem_Init(10000);
    printf("Curr Size: %d\n", root_len->data);
    printf("Size of header: %lu\n\n", 2 * sizeof(node));
    Tree_Dump();
    void * ptr1 = Mem_Alloc(400, WORSTFIT);
    Tree_Dump();
    void * ptr2 = Mem_Alloc(40, WORSTFIT);
    Tree_Dump();
    //printf("Mem Location 1: %p\n", ptr1);
    Mem_Free(ptr1);
    Tree_Dump();
    void * ptr3 = Mem_Alloc(10, WORSTFIT);
    Tree_Dump();
    Mem_Free(ptr2);
    Tree_Dump();
}
