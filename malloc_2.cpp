//
// Created by teich on 26/06/2023.
//
#define MAXSIZE 100000000
#include <cstring>
#include <unistd.h>


//TODO: address structure for meta_data - theirs or like ATAM?
struct MallocMetadata {
    size_t size;
    bool is_free;
    MallocMetadata* next;
    MallocMetadata* prev;
};


//TODO: global sorted LIST (by address) - our
MallocMetadata* sorted_list = NULL;
//TODO: global counter_total_blocks
int counter_total_blocks = 0;



void* smalloc(size_t size){
    // check size
    if (size == 0 || size > MAXSIZE){
        return NULL;
    }

    // move on the "free_list"
    MallocMetadata* temp = sorted_list;
    // search for fit block
    while (temp != NULL){
        if (temp->is_free == true && temp->size>=size){
            // if found - allocate it
            temp->is_free=false;
            // update the counters by the size of the block (not by size 10/1000 example)

            // return pointer to start
            return temp;
        }
        // else size is not enough
        else{
            temp = temp->next;
        }
    }

    // else if  - sbrk
    // new_block holds the address of the metadata
    MallocMetadata* new_block = sbrk(size + sizeof(MallocMetadata));

    //if sbrk succeeded
    if(new_block != (void*)(-1)){

        //create the metaData for the allocation
        MallocMetadata metadata;
        metadata.size=size;
        metadata.is_free=false;
        metadata.prev=NULL;
        metadata.next=NULL;

        // put the metadata struct in the place we allocate - convert the address from void* to metadata* and saved in new_block the metadata
        *new_block = metadata;

        // add to the allocations list

        // if first block - update the global list pointer + counter
        if(sorted_list==NULL){
            sorted_list = new_block;
        }

        // create temp
        MallocMetadata* head = sorted_list;
        MallocMetadata* temp = head;

        // run the allocated blocks and find the right spot to enter the new_block
        while (temp!=NULL) {
            // if need to be before temp
            if (&new_block < &temp) {
                // save the temp_prev
                MallocMetadata *temp_prev = temp->prev;

                // update the prev to point on the new_block
                temp_prev->next = new_block;

                // update the new_block to point on the curr from back and on the prev curr.prev from back
                new_block->prev = temp_prev;
                new_block->next = temp;

                // update the curr.prev to point on new_block
                temp->prev = new_block;

                // if enter before the first one - update the global list pointer
                if (head==temp){
                    sorted_list = new_block;
                }

                // found the right spot so can break the loop
                break;
            }

            // if address of new_block is the largest, enter at the end
            else if (temp->next==NULL && (&new_block > &temp)){
                temp->next = new_block;
                new_block->prev - temp;
                break;
            }

            else {
                // advance temp
                temp = temp->next;
            }
        }

        // increase the amount of blocks
        counter_total_blocks++;

        // return pointer to start of block
        return ((void*)((MallocMetadata*)new_block + sizeof(MallocMetadata)));
    }

    // else (sbrk fail)
    return NULL;
}


void* scalloc(size_t num, size_t size){
    //like smalloc with ZERO

    // call smalloc
    void* new_block = smalloc(num * size);
    if (new_block == NULL){
        return NULL;
    }
    // if not NULL memset 0
    std::memset(new_block, 0, num * size);

    return new_block;
}

void sfree(void* p){
    // check if p is null or meta_data flag is free (p-size(meta_data))
    if (p == NULL){
        return;
    }
    // else - free means get the right address and update flag
    MallocMetadata* to_free = (MallocMetadata*)p - sizeof(MallocMetadata);
    to_free->is_free= true;
}

void* srealloc(void* oldp, size_t size){
    // check size and pointer
    if (size == 0 || size > MAXSIZE){
        return NULL;
    }
    void* result;

    // if oldp == NULL allocate size bytes and return
    if (oldp == NULL){
        result = smalloc(size);
        if (result!=NULL){
            return result;
        }
        return NULL;
    }

    // if size < oldp.size() return oldp
    MallocMetadata* old_block = (MallocMetadata*)oldp-sizeof(MallocMetadata);
    if (size <= old_block->size){
        return oldp;
    }
    // else need to get new allocation
    else{
        // result = smalloc()
        result = smalloc(size);
        // if allocation fail - return NULL and dont sfree the oldp
        if  (result == NULL){
            return NULL;
        }
        // else - allocation succeeded
        else {
            // copy the content - with std::memmove()
            std::memmove(result, oldp, old_block->size);
            // sfree oldp
            sfree(oldp);
            // return
            return result;
        }
    }
}

size_t _num_free_blocks(){
    // init counter
    size_t counter = 0;
    // move on the list
    MallocMetadata* curr = sorted_list;
    // for every block isfree==true make counter++
    while (curr != NULL){
        if (curr->is_free==true){
            counter++;
        }
        curr = curr->next;
    }
    // return counter
    return counter;
}

size_t _num_free_bytes(){
    // like free_block but instead of return the amount of them, return the amount of sizes

    // init total_free_space
    size_t total_free_space = 0;
    // move on the list
    MallocMetadata* curr = sorted_list;
    // for every block isfree==true make += total_free_space
    while (curr != NULL){
        if (curr->is_free==true){
            total_free_space += curr->size;
        }
        curr = curr->next;
    }
    // return total_free_space
    return total_free_space;
}

size_t _num_allocated_blocks(){
    //return counter_total_blocks
    return counter_total_blocks;
}

size_t _num_allocated_bytes(){
    // like free_byte without the condition of is_free == true

    // init total_space
    size_t total_space = 0;
    // move on the list
    MallocMetadata* curr = sorted_list;
    // for every block isfree==true make += total_space
    while (curr != NULL){
        total_space += curr->size;
        curr = curr->next;
    }
    // return total_space
    return total_space;
}

size_t _num_meta_data_bytes(){
    //return counter_total_blocks*sizeof(meta_data)
    return counter_total_blocks*sizeof(MallocMetadata);
}

size_t _size_meta_data(){
    //return sizeof(meta_data)
    return sizeof(MallocMetadata);
}


int main(){
    void* a = smalloc(0);
    void* b = smalloc(100000001);
    void* c = smalloc(100000000);
    void* d = smalloc(99999999);
    printf("a: %u\n", (unsigned int)a);
    printf("b: %u\n", (unsigned int)b);
    printf("c: %u\n", (unsigned int)c);
    printf("d: %u\n", (unsigned int)d);
    return 0;
}