#include "mem.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <stdlib.h>


#define _debug
//#define _bestfit
//#define _worstfit
#define LOCATION_SORTED (0)
#define LENGTH_SORTED (1)

struct Node {
    int             data; //size
    struct Node     *next;
};

typedef struct Node node;

int                 m_error;
int                 init_called = 0;
void                *start_of_region;
node                *header;


void Insert_Node(node **toInsert, node **n) {
    printf("insert node called\n");
    if(*n == NULL) {
        *n = *toInsert;
    } else {
        node *curr = *n;
        node *insert = *toInsert;
        //handle case with one element in list
        if (curr->next == NULL) {
            if (curr < insert) {
                curr->next = insert;
            } else {
                *n = insert;
                insert->next = curr;
            }
        }

        while (curr->next != NULL) {
            if (insert > curr && insert < curr->next) {
                insert->next = curr->next;
                curr->next = insert;
                return;
            }
            curr = curr->next;
        }
    }
}

node* Get_Best_Fit(int sizeToFit, node **n) {
    printf("get best fit called with size: %i.\n", sizeToFit);
    if ((*n) == NULL) {
        printf("Null header");
        //handle error
        return NULL;
    }
    node *curr = *n;
    node *curr_best = NULL;
    int curr_best_difference = sizeToFit;
    printf("curr size: %i", curr->data);
    if (curr->data >= sizeToFit) {
        curr_best = curr;
        curr_best_difference = curr->data - sizeToFit;
    }
    
    while (curr->next != NULL) {
        curr = curr->next;
        printf("curr size: %i", curr->data);
        if (curr->data >= sizeToFit && ((curr->data - sizeToFit) < curr_best_difference)) {
            curr_best = curr;
            curr_best_difference = curr->data - sizeToFit;
        }
    }
    printf("returning %p: ", curr_best);
    return curr_best;
}

node* Get_Worst_Fit(int sizeToFit, node **n) {
    printf("get worst fit called.\n");
    if ((*n) == NULL) {
        //handle error
        return NULL;
    }
    node *curr = *n;
    node *curr_worst = NULL;
    int curr_worst_difference = sizeToFit;
    
    if (curr->data > sizeToFit) {
        curr_worst = curr;
        curr_worst_difference = curr->data - sizeToFit;
    }
    
    while (curr->next != NULL) {
        curr = curr->next;
        if (curr->data > sizeToFit && ((curr->data - sizeToFit) > curr_worst_difference)) {
            curr_worst = curr;
            curr_worst_difference = curr->data - sizeToFit;
        }
    }
    
    return curr_worst;
}

node* Get_First_Fit(int sizeToFit, node **n) {
    printf("get first fit called.\n");
    if ((*n) == NULL) {
        //handle error
        return NULL;
    }
    node *curr = *n;
    
    if (curr->data > sizeToFit) {
        return curr;
    }
    
    while (curr->next != NULL) {
        curr = curr->next;
        if (curr->data > sizeToFit) {
            return curr;
        }
    }
    
    return NULL; //throw error on no fits!!
}


void Delete_Node(void * address, node **n) {
    printf("delete node called\n");
    if (*n == NULL) printf("Error: null header.");
    node *curr = *n;
    node *tmp;
    if (curr == address) {
        *n = NULL;
    }
    while (curr->next != NULL) {
        if (curr->next == address) {
            tmp = curr->next;
            if (curr->next->next != NULL) {
                curr->next = curr->next->next;
            } else {
                curr->next = NULL;
            }
            return;
        }
        curr = curr->next;
    }
}

void Display(node **n) {
    node *curr = *n;
    printf("%p | size: %d\n", curr, curr->data);
    while(curr->next != NULL) {
        curr = curr->next;
        printf("%p | size: %d\n", curr, curr->data);
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
    header = start_of_region;
    printf("root_len: %p\n", header);
    header->data = size_of_region;
    
    // close the device (don't worry, mapping should be unaffected)
    close(fd);
    init_called = 1;
    return 0;
}

void *Mem_Alloc(int size, int style){
    printf("-----ALLOC CALLED WITH SIZE: %i\n\n", (int)(size + sizeof(node)));
    
    node* to_remove;
    node* new_free;
    if (size % 8 != 0) {
        if (size < 8) {
            size = 8;
        } else {
            size = (size / 8 + 1) * 8;
        }
    }
    int total_size_free_space;
	size = (int)(size + sizeof(node)); //each memory locations
    
    // used for temporarily allowing arbitrary number of bytes to be added to ptr
    char* temp_ptr;
    switch (style){
        case BESTFIT:
            
            to_remove = Get_Best_Fit(size, &header);
            if (to_remove == NULL) {
                m_error = E_NO_SPACE;
                return to_remove;
                break;
            }
            printf("to remove: %p", to_remove);
            //printf("memhead: %p", to_remove->mem_head);
            Delete_Node(to_remove, &header);
            
            total_size_free_space = to_remove->data;
            
            to_remove->data = size;

            if (total_size_free_space - size > 0) {
                // move the pointer to the beginning of the new free space
                temp_ptr = (char*) to_remove;
                temp_ptr += size; 
                new_free = (node*) temp_ptr;
                new_free->data = total_size_free_space - size;
                new_free->next = to_remove->next;
				Insert_Node(&new_free, &header);
            }
            
            return to_remove;
            break;
    
        case WORSTFIT:
            to_remove = Get_Worst_Fit(size, &header);
            if (to_remove == NULL) {
                m_error = E_NO_SPACE;
                return to_remove;
                break;
            }
            Delete_Node(to_remove, &header);
            
            total_size_free_space = to_remove->data;
            
            to_remove->data = size;
            
            if (total_size_free_space - size > 0) {
                // move the pointer to the beginning of the new free space
                temp_ptr = (char*) to_remove;
                temp_ptr += size;
                new_free = (node*) temp_ptr;
                new_free->data = total_size_free_space - size;
                new_free->next = to_remove->next;
				Insert_Node(&new_free, &header);
            }
            
            return to_remove;
            break;

        case FIRSTFIT:
            
            to_remove = Get_First_Fit(size, &header);
            if (to_remove == NULL) {
                m_error = E_NO_SPACE;
                return to_remove;
                break;
            }
            Delete_Node(to_remove, &header);
            
            total_size_free_space = to_remove->data;
            
            to_remove->data = size;
            
            if (total_size_free_space - size > 0) {
                // move the pointer to the beginning of the new free space
                temp_ptr = (char*) to_remove;
                temp_ptr += size;
                new_free = (node*) temp_ptr;
                new_free->data = total_size_free_space - size;
                new_free->next = to_remove->next;
				Insert_Node(&new_free, &header);
            }
            
            return to_remove;
            break;
            
        }
    return NULL;
}

int Mem_Free(void *ptr){
    //non-coalescing
    if (ptr == NULL) return -1;
    printf("FREEING MEMEORY\n");
    node* tmpPtr = (node*) ptr;
    Insert_Node(&tmpPtr, &header);
    return 0;
}

void Mem_Dump(){

}

void Tree_Dump() {
    printf("PRINT: \n");
    Display(&header);
    printf("\n");
}

#ifdef _debug 
int main(){
    assert(Mem_Init(4096) == 0);
    void *ptr[4];
    void *first, *best, *worst;
    
    assert(Mem_Alloc(8, FIRSTFIT) != NULL);
    ptr[0] = Mem_Alloc(40, FIRSTFIT);
    assert(Mem_Alloc(8, FIRSTFIT) != NULL);
    ptr[1] = Mem_Alloc(56, FIRSTFIT);
    assert(Mem_Alloc(8, FIRSTFIT) != NULL);
    first = Mem_Alloc(256, FIRSTFIT);
    assert(Mem_Alloc(8, FIRSTFIT) != NULL);
    best = Mem_Alloc(128, FIRSTFIT);
    assert(Mem_Alloc(8, FIRSTFIT) != NULL);
    ptr[2] = Mem_Alloc(32, FIRSTFIT);
    assert(Mem_Alloc(8, FIRSTFIT) != NULL);
    worst = Mem_Alloc(512, FIRSTFIT);
    assert(Mem_Alloc(8, FIRSTFIT) != NULL);
    ptr[3] = Mem_Alloc(32, FIRSTFIT);
    
    while(Mem_Alloc(128, FIRSTFIT) != NULL);
    assert(m_error == E_NO_SPACE);
    
    assert(Mem_Free(ptr[2]) == 0);
    assert(Mem_Free(ptr[3]) == 0);
    assert(Mem_Free(first) == 0);
    assert(Mem_Free(best) == 0);
    assert(Mem_Free(ptr[1]) == 0);
    assert(Mem_Free(worst) == 0);
    assert(Mem_Free(ptr[0]) == 0);
    
    assert(Mem_Alloc(128, BESTFIT) == best);
    
    exit(0);
}
#endif