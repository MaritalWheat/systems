#include "mem.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>


#define _debug
//#define _bestfit
//#define _worstfit
#define LOCATION_SORTED (0)
#define LENGTH_SORTED (1)

struct TreeNode {
    int             data;

    void            *mem_head; //always want to be able to point to base of memory chunk
    struct TreeNode *left_child;
    struct TreeNode *right_child;
};

typedef struct TreeNode node;

int                 m_error;
int                 init_called = 0;
void                *start_of_region;
node                *root_loc;
node                *root_len;


void Insert_Node_Len(node **toInsert, node **n) {
    if(*n == NULL) {
        *n = *toInsert;
    } else {
        if((*toInsert)->data > (*n)->data) {
            Insert_Node_Len(&(*toInsert), &(*n)->right_child);
        } else {
            Insert_Node_Len(&(*toInsert), &(*n)->left_child);
        }
    }
}

void Insert_Node_Loc(node **toInsert, node **n) {
    if(*n == NULL) {
        *n = *toInsert;
    } else {
        if((*n)->mem_head == (*toInsert)->mem_head)
        {
            // handle error?
        }
        else
        {
            if((*toInsert)->mem_head > (*n)->mem_head)
                Insert_Node_Loc(&(*toInsert), &(*n)->right_child);
            else
                Insert_Node_Loc(&(*toInsert), &(*n)->left_child);
        }
    }
}

//check for null return value when calling!!
node* Search_For_Node(void* address, node **n) {
    if((*n)->mem_head == address) {
        return (*n);
    }
    //this is pretty dangerous, never checking if value passed in is null...
    if ((*n)->left_child != NULL) {
        Search_For_Node(address, &((*n)->left_child));
    } else if ((*n)->right_child != NULL) {
        Search_For_Node(address, &((*n)->right_child));
    }
}


node* Get_Best_Fit(int sizeToFit, node **n) {
    
    if ((*n) == NULL) return;
    
    if ((*n)->data == sizeToFit) return (*n);
    
    if ((*n)->data > sizeToFit) {
        if ((*n)->left_child != NULL) {
            if ((*n)->left_child->data >= sizeToFit) {
                Get_Best_Fit(sizeToFit, &((*n)->left_child));
            } else {
                return (*n);
            }
        } else {
            return (*n);
        }
    } else {
        if ((*n)->right_child != NULL) {
            Get_Best_Fit(sizeToFit, &((*n)->right_child));
        }
    }
    //return (*n);
}

node* Get_Worst_Fit(int sizeToFit, node **n) {
    
    if ((*n) == NULL) return;
    
    if ((*n)->data > sizeToFit) {
        if ((*n)->right_child != NULL) {
            Get_Worst_Fit(sizeToFit, &((*n)->right_child));
        } else {
            return (*n);
        }
    } else if ((*n)->data == sizeToFit) {
        return (*n);
    } else {
        if ((*n)->right_child != NULL) {
            Get_Worst_Fit(sizeToFit, &((*n)->right_child));
        } else {
            //error case
        }
    }
}

node* Get_First_Fit(int sizeToFit, node **n) {
    if ((*n) == NULL) return NULL;
    if((*n)->left_child != NULL) {
        if ((*n)->left_child->data > sizeToFit) {
            Get_First_Fit(sizeToFit, &((*n)->left_child));
        }
    } else if ((*n)->data > sizeToFit) {
        return (*n);
    } else {
        //error case
    }
}

void Delete_Node_Len(void * address, node **n, int data) {
    if(*n == NULL)
        printf("\nValue does not exist in tree!\n");
    else
        if((*n)->data == data) {
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
            return;
        }
        else
            if(data > (*n)->data)
                Delete_Node_Len(address, &(*n)->right_child, data);
            else
                Delete_Node_Len(address, &(*n)->left_child, data);
}

void Delete_Node_Loc(void *address, node **n) {
    if(*n == NULL) {
        printf("\nValue does not exist in tree!\n");
    } else {
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
            return;
        }
        else
            if(address > (*n)->mem_head)
                Delete_Node_Loc(address, &((*n)->right_child));
            else
                Delete_Node_Loc(address, &((*n)->left_child));
    }
}

void Display_In_Order(node **n) {
    if((*n) != NULL) {
        Display_In_Order(&((*n)->left_child));
        printf("%p | size: %d\n", (*n)->mem_head, (*n)->data);
        Display_In_Order(&((*n)->right_child));
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
    root_loc = start_of_region + sizeof(node);
    root_loc->data = size_of_region;
    root_loc->mem_head = root_len;
    printf("root_loc: %p\n", root_loc);

    // close the device (don't worry, mapping should be unaffected)
    close(fd);
    init_called = 1;
    return 0;
}

void *Mem_Alloc(int size, int style){
    printf("-----ALLOC CALLED WITH SIZE: %i\n\n", (int)(size + 2 * sizeof(node)));
    node* to_remove_len;
    node* to_remove_loc;
    node* new_free_len;
    node* new_free_loc;

    int total_size_free_space;
	size = (int)(size + 2 * sizeof(node)); //each memory locations needs a len & loc sorted node
    
    // used for temporarily allowing arbitrary number of bytes to be added to ptr
    char* temp_ptr;
    switch (style){
        case BESTFIT:
            
            to_remove_len = Get_Best_Fit(size, &root_len);
            to_remove_loc = Search_For_Node(to_remove_len->mem_head, &root_loc);
            
            if (to_remove_loc != NULL) {
                Delete_Node_Loc(to_remove_loc->mem_head, &root_loc);
            }
            
            Delete_Node_Len(to_remove_loc->mem_head, &root_len, to_remove_loc->data);
            total_size_free_space = to_remove_len->data;
            
            to_remove_len->data = size;
            to_remove_loc->data = size;

            if ( total_size_free_space - size > 0){
                // move the pointer to the beginning of the new free space
                temp_ptr = (char*) to_remove_len;
                temp_ptr += size; 
                new_free_len = (node*) temp_ptr;
                new_free_len->data = total_size_free_space - size;
                new_free_len->mem_head = new_free_len;
                temp_ptr += sizeof(node);
                new_free_loc = (node*)(temp_ptr);
                new_free_loc->data = new_free_len->data;
                new_free_loc->mem_head = new_free_len->mem_head;
				Insert_Node_Len(&new_free_len, &root_len);
                Insert_Node_Loc(&new_free_loc, &root_loc);
            }
            
            return to_remove_len;
            break;
            
        case WORSTFIT:
            to_remove_len = Get_Worst_Fit(size, &root_len);
            to_remove_loc = Search_For_Node(to_remove_len->mem_head, &root_loc);
            
            if (to_remove_loc != NULL) {
                Delete_Node_Loc(to_remove_loc->mem_head, &root_loc);
            }
            Delete_Node_Len(to_remove_loc->mem_head, &root_len, to_remove_loc->data);

            total_size_free_space = to_remove_len->data;
            
            to_remove_len->data = size;
            to_remove_loc->data = size;
            if ( total_size_free_space - size > 0){
                // move the pointer to the beginning of the new free space
                temp_ptr = (char*) to_remove_len;
                temp_ptr += size;
                new_free_len = (node*) temp_ptr;
                new_free_len->data = total_size_free_space - size;
                new_free_len->mem_head = new_free_len;
                temp_ptr += sizeof(node);
                new_free_loc = (node*)(temp_ptr);
                new_free_loc->data = total_size_free_space - size;
                new_free_loc->mem_head = new_free_len->mem_head;
				Insert_Node_Len(&new_free_len, &root_len);
                Insert_Node_Loc(&new_free_loc, &root_loc);
            }
            
            return to_remove_len;
            break;

        case FIRSTFIT:
            
            to_remove_loc = Get_First_Fit(size, &root_loc);
            
            if (to_remove_loc == NULL) break;
            
            Delete_Node_Len(to_remove_loc->mem_head, &root_len, to_remove_loc->data);            
            to_remove_len = (node*) to_remove_loc->mem_head;
            
            Delete_Node_Loc(to_remove_loc->mem_head, &root_loc);            
            total_size_free_space = to_remove_loc->data;
            
            to_remove_loc->data = size;
            to_remove_len->data = size;

            if (total_size_free_space - size > 0){
                // move the pointer to the beginning of the new free space
                temp_ptr = (char*) to_remove_loc->mem_head;
                temp_ptr += size;
                new_free_len = (node*) temp_ptr;
                new_free_len->data = total_size_free_space - size;
                new_free_len->mem_head = new_free_len;
                new_free_loc = new_free_len->mem_head + sizeof(node);
                new_free_loc->data = total_size_free_space - size;
                new_free_loc->mem_head = new_free_len->mem_head;
				Insert_Node_Len(&new_free_len, &root_len);
                Insert_Node_Loc(&new_free_loc, &root_loc);
            }
            
            return to_remove_loc->mem_head;
            break;
        }
}

node* Back_Check_Helper(void* address, node **n) {
    void* offset_pointer = (void*)(((*n)->mem_head)) + (((node*)(*n))->data);
    if(offset_pointer == address) {
        return (*n);
    }
    //this is pretty dangerous, never checking if value passed in is null...
    if ((*n)->left_child != NULL) {
        Search_For_Node(address, &((*n)->left_child));
    } else if ((*n)->right_child != NULL) {
        Search_For_Node(address, &((*n)->right_child));
    }
}

int Mem_Free(void *ptr){
    printf("-------MEM FREE TRACE-----------");
    int check_forward = 1;
    void* forward_pointer = ptr;
    printf("    Original ptr: %p\n", ptr);
    forward_pointer += ((node*)ptr)->data;
    while (check_forward != 0) {
        node* search_result = Search_For_Node(forward_pointer, &root_len);
        printf("    Search Result Forward: %p\n", search_result);
        if (search_result != NULL) {
            node* casted_forward_pointer = (node*)forward_pointer;
            Delete_Node_Len(forward_pointer, &root_len, ((node*)(forward_pointer))->data);
            Delete_Node_Loc(forward_pointer, &root_loc);
            ((node*)(ptr))->data += casted_forward_pointer->data;
            forward_pointer += casted_forward_pointer->data;
        } else {
            printf("    Breaking forward search!\n");
            check_forward = 0;
        }
    }
    
    printf("    Forward coalesced ptr: %p | %i\n", ptr, ((node*)(ptr))->data);
    
    int check_back = 1;
    int no_insert = 0;
    void* back_pointer;
    while (check_back != 0) {
        node* back_check_result = Back_Check_Helper(ptr, &root_len);
        printf("    Search Result Backward: %p\n", back_check_result);
        if (back_check_result == NULL) {
            printf("    Breaking backward search!\n");
            check_back = 0;
        } else {
            //Delete_Node_Len(((node*)(ptr))->mem_head, &root_len, ((node*)(ptr))->data);
            //Delete_Node_Loc(((node*)(ptr))->mem_head, &root_loc);
            back_check_result->data += ((node*)(ptr))->data;
            void* loc_updater = back_check_result->mem_head + sizeof(node);
            ((node*)(loc_updater))->data += ((node*)(ptr))->data;
            no_insert = 1;
            ptr = (void*)back_check_result;

        }
    }
    printf("    Back coalesced ptr: %p | %i\n", ptr, ((node*)(ptr))->data);
    
    printf("-----FREE CALLED WITH ADDRESS: %p\n\n", ptr);
    if (no_insert == 0) {
        node* tmpPtr = (node*) ptr;
        Insert_Node_Len(&tmpPtr, &root_len);
        ptr += sizeof(node);
        node* tmpPtr2 = (node*) ptr;
        Insert_Node_Loc(&tmpPtr2, &root_loc);
    }
}

void Mem_Dump(){

}

void Tree_Dump() {
    printf("PRINT BY LENGTH: \n");
    Display_In_Order(&root_len);
    printf("\nPRINT BY LOC: \n");
    Display_In_Order(&root_loc);
    printf("\n");
}

#ifdef _debug 
int main(){
    #ifdef _bestfit 
        printf("BESTFIT------------------------\n");
        Mem_Init(10000);
        printf("    Curr Size: %d\n", root_len->data);
        printf("    Size of header: %lu\n\n", 2 * sizeof(node));
        Tree_Dump();
        void * ptr1 = Mem_Alloc(400, BESTFIT);
        Tree_Dump();
        void * ptr2 = Mem_Alloc(40, BESTFIT);
        Tree_Dump();
        Mem_Free(ptr1);
        Tree_Dump();
        void * ptr3 = Mem_Alloc(10, BESTFIT);
        Tree_Dump();
        Mem_Free(ptr2);
        Tree_Dump();
    #elif defined _worstfit
        printf("WORSTFIT------------------------\n");
        Mem_Init(10000);
        printf("    Curr Size: %d\n", root_len->data);
        printf("    Size of header: %lu\n\n", 2 * sizeof(node));
        Tree_Dump();
        void * ptr1 = Mem_Alloc(400, WORSTFIT);
        Tree_Dump();
        void * ptr2 = Mem_Alloc(40, WORSTFIT);
        Tree_Dump();
        Mem_Free(ptr1);
        Tree_Dump();
        void * ptr3 = Mem_Alloc(10, WORSTFIT);
        Tree_Dump();
        Mem_Free(ptr2);
        Tree_Dump();
    #else
        printf("FIRSTFIT------------------------\n");
        Mem_Init(10000);
        printf("    Curr Size: %d\n", root_len->data);
        printf("    Size of header: %lu\n\n", 2 * sizeof(node));
        Tree_Dump();
        void * ptr1 = Mem_Alloc(400, FIRSTFIT);
        Tree_Dump();
        void * ptr2 = Mem_Alloc(40, FIRSTFIT);
        Tree_Dump();
        Mem_Free(ptr1);
        Tree_Dump();
        void * ptr3 = Mem_Alloc(10, FIRSTFIT);
        Tree_Dump();
        Mem_Free(ptr2);
        Tree_Dump();
    #endif
}
#endif