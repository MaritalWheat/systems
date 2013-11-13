#include "mem.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <stdlib.h>


//#define _debug

struct Node {
    int             data; //size
    int             wizard; //sanity check
    struct Node     *next;
};

typedef struct Node node;

int                 m_error;
int                 init_called = 0;
void                *start_of_region;
node                *header;


void Insert_Node(node **toInsert, node **n) {
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
                (*n)->next = curr;
            }
            return;
        }
        //handle insertion at head with multi-element list
        if (insert < curr) {
            *n = insert;
            (*n)->next = curr;
            return;
        }
        
        while (curr->next != NULL) {
            if (insert > curr && insert < curr->next) {
                insert->next = curr->next;
                curr->next = insert;
                return;
            }
            //handle ende of list
            if (curr->next->next == NULL) {
                if (insert > curr->next) {
                    curr->next->next = insert;
                } else {
                    insert->next = curr->next;
                    curr->next = insert;
                }
                return;
            }
            curr = curr->next;
        }
    }
}

node* Search_For_Node(void *address, node **n) {
    if (*n == NULL) {
        printf("     Error: null header.\n");
        return NULL;
    }
    node *curr = *n;
    if (curr == address) {
        return curr;
    }
    while (curr->next != NULL) {
        if (curr->next == address) {
            return curr->next;        }
        curr = curr->next;
    }
    return NULL;
}

node* Search_For_Previous_Node(void *address, node **n, int size) {
    if (*n == NULL) {
        return NULL;
    }
    node *curr = *n;
    if (curr == address) {
        return NULL;
    }
    
    void *nextAddress = curr;
    size = curr->data;
    nextAddress += size;
    
    while (curr->next != NULL) {
        if (nextAddress == address) {
            return curr;        }
        curr = curr->next;
        nextAddress = curr;
        size = curr->data;
        nextAddress += size;
    }
    return NULL;
}

node* Get_Best_Fit(int sizeToFit, node **n) {
    if ((*n) == NULL) {
        //handle error
        return NULL;
    }
    node *curr = *n;
    node *curr_best = NULL;
    int curr_best_difference = 999999;
    
    if (curr->data >= sizeToFit) {
        curr_best = curr;
        curr_best_difference = curr->data - sizeToFit;
    }
    
    while (curr->next != NULL) {
        curr = curr->next;
        if (curr->data >= sizeToFit && ((curr->data - sizeToFit) < curr_best_difference)) {
            curr_best = curr;
            curr_best_difference = curr->data - sizeToFit;
        }
    }
    
    return curr_best;
}

node* Get_Worst_Fit(int sizeToFit, node **n) {
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
    if (*n == NULL) printf("Error: null header.\n");
    node *curr = *n;
    node *tmp;
    if (curr == address) {
        *n = curr->next;
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
    if (*n == NULL) {
        printf("Null header - print call.\n");
        return;
    }
    node *curr = *n;
    while(curr->next != NULL) {
        curr = curr->next;
    }
}

void Mem_Dump(){
    printf("PRINT: -----------\n");
    Display(&header);
    printf("\n");
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
    header->wizard = -666;
    
    // close the device (don't worry, mapping should be unaffected)
    close(fd);
    init_called = 1;
    return 0;
}

void *Mem_Alloc(int size, int style){
    node* to_remove;
    node* new_free;
    void* return_ptr;
    
    //8-byte align for performance
    if (size % 8 != 0) {
        if (size < 8) {
            size = 8;
        } else {
            size = (size / 8 + 1) * 8;
        }
    }
    
    int total_size_free_space;
    size = (int)(size + sizeof(node)); //actual size needed is desired size + node size
    char* temp_ptr; //used to move forward during memory splitting
    
    //allocation handling, based on desired style
    switch (style){
        case BESTFIT:
            
            to_remove = Get_Best_Fit(size, &header);
            if (to_remove == NULL) {
                m_error = E_NO_SPACE;
                return to_remove;
                break;
            }
            
            Delete_Node(to_remove, &header);
            
            total_size_free_space = to_remove->data;
            
            to_remove->data = size;

            if (total_size_free_space - size > 0) {
                temp_ptr = (char*) to_remove;
                temp_ptr += size; 
                new_free = (node*) temp_ptr;
                new_free->data = total_size_free_space - size;
                new_free->next = to_remove->next;
                new_free->wizard = -666;
                Insert_Node(&new_free, &header);
            }
    
            return_ptr = (void*) to_remove;
            return_ptr += sizeof(node);
            return return_ptr;
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
                temp_ptr = (char*) to_remove;
                temp_ptr += size;
                new_free = (node*) temp_ptr;
                new_free->data = total_size_free_space - size;
                new_free->next = to_remove->next;
                new_free->wizard = -666;
                Insert_Node(&new_free, &header);
            }
   
            return_ptr = (void*) to_remove;
            return_ptr += sizeof(node);
            return return_ptr;
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
                new_free->wizard = -666;
                Insert_Node(&new_free, &header);
            }
            
            return_ptr = (void*) to_remove;
            return_ptr += sizeof(node);
            return return_ptr;
            break;
            
        }
    return NULL;
}


int Mem_Free(void *ptr){
    
    //check edge cases
    if (ptr == NULL) return -1;
    ptr -= sizeof(node);
    node *tmpPtr = (node*) ptr;
    if (tmpPtr->wizard != -666) {
        m_error = E_BAD_POINTER;
        return -1;
    }
    
    //forward check - coalescion
    void *check_address = ptr + tmpPtr->data;
    node *forward_node;
    int check_forward = 1;
    while(check_forward != 0) {
        forward_node = Search_For_Node(check_address, &header);
        if (forward_node == NULL) {
            check_forward = 0;
            break;
        } else {
            if (forward_node->next != NULL) {
                tmpPtr->next = forward_node->next;
            } else {
                tmpPtr->next = NULL;
            }
            Delete_Node(forward_node, &header);
            check_address = check_address + forward_node->data;
            tmpPtr->data += forward_node->data;
        }
    }
    
    //back check - coalescion
    check_address = ptr;
    node *back_node;
    int size = tmpPtr->data;
    int check_back = 1;
    while(check_back != 0) {
        back_node = Search_For_Previous_Node(check_address, &header, size);
        if (back_node == NULL) {
            check_back = 0;
            break;
        } else {
            Delete_Node(back_node, &header);
            if (tmpPtr->next != NULL) {
                back_node->next = tmpPtr->next;
            } else {
                back_node->next = NULL;
            }

            check_address = check_address - back_node->data;
            back_node->data += tmpPtr->data;
            size = back_node->data;
            tmpPtr = back_node;
        }
    }
    
    //insert freed segment back into free list
    tmpPtr->wizard = -666;
    Insert_Node(&tmpPtr, &header);
    return 0;
}

int main(){
    #ifdef _debug 
    //test cases
    #endif
    exit(0);
}
