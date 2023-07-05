//
// Created by teich on 26/06/2023.
//
#define MAXSIZE 100000000
#include <cstring>
#include <unistd.h>
#include <iostream>


struct MallocMetadata {
    size_t size;
    bool is_free;
    MallocMetadata* next;
    MallocMetadata* prev;
};


MallocMetadata* sorted_list = nullptr;
int counter_total_blocks = 0;


void add_to_list(void* new_block){
    MallocMetadata* Malloc_new_block = (MallocMetadata*)new_block;

    // if first block - update the global list pointer + counter
    if(sorted_list==nullptr){
        sorted_list = Malloc_new_block;
        counter_total_blocks++;
        return;
    }

    // create temp
    MallocMetadata* head = sorted_list;
    MallocMetadata* temp = head;

    // run the allocated blocks and find the right spot to enter the Malloc_new_block
    while (temp!=nullptr) {
        // if need to be before temp
        if (Malloc_new_block < temp) {
            // save the temp_prev
            MallocMetadata *temp_prev = temp->prev;

            // update the prev to point on the Malloc_new_block
            if (temp_prev != nullptr){
                temp_prev->next = Malloc_new_block;
            }
            // if temp_prev == nullptr - means temp was the first elem
            else {
                sorted_list = Malloc_new_block;
            }

            // update the Malloc_new_block to point on the curr from back and on the prev curr.prev from back
            Malloc_new_block->prev = temp_prev;
            Malloc_new_block->next = temp;

            // update the curr.prev to point on Malloc_new_block
            temp->prev = Malloc_new_block;

            // if enter before the first one - update the global list pointer
            if (head==temp){
                sorted_list = Malloc_new_block;
            }

            // found the right spot so can break the loop
            break;
        }

            // if address of Malloc_new_block is the largest, enter at the end
        else if (temp->next==nullptr){
            temp->next = Malloc_new_block;
            Malloc_new_block->prev = temp;
            break;
        }

        else {
            // advance temp
            temp = temp->next;
        }
    }

    // increase the amount of blocks
    counter_total_blocks++;
    return;
}

void* find_free_block(size_t size){
    // move on the "free_list"
    MallocMetadata* temp = sorted_list;
    // search for fit block
    while (temp != nullptr){
        if (temp->is_free == true && temp->size>=size){
            // if found - allocate it
            temp->is_free=false;
            // update the counters by the size of the block (not by size 10/1000 example)

            // return pointer to start
            return (void*)(temp+1);
        }
            // else size is not enough
        else{
            temp = temp->next;
        }
    }
    return nullptr;
}


void* smalloc(size_t size){
    // check size
    if (size == 0 || size > MAXSIZE){
        return nullptr;
    }

    void* new_block = find_free_block(size);
    if (new_block!=nullptr){
        return new_block;
    }

    // else if  - sbrk
    // new_block holds the address of the metadata
    new_block = sbrk(size + sizeof(MallocMetadata));

    //if sbrk succeeded
    if(new_block != (void*)(-1)){

        //create the metaData for the allocation
        MallocMetadata metadata;
        metadata.size=size;
        metadata.is_free=false;
        metadata.prev=nullptr;
        metadata.next=nullptr;

        // put the metadata struct in the place we allocate - convert the address from void* to metadata* and saved in new_block the metadata
        *(MallocMetadata*)new_block = metadata;

        // add to the allocations list
        add_to_list(new_block);

        // return pointer to start of block
        return ((void*)((MallocMetadata*)new_block + 1));
    }

    // else (sbrk fail)
    return nullptr;
}


void* scalloc(size_t num, size_t size){
    //like smalloc with ZERO

    // call smalloc
    void* new_block = smalloc(num * size);
    if (new_block == nullptr){
        return nullptr;
    }
    // if not nullptr memset 0
    std::memset(new_block, 0, num * size);

    return new_block;
}

void sfree(void* p){
    // check if p is nullptr or meta_data flag is free (p-size(meta_data))
    if (p == nullptr){
        return;
    }
    // else - free means get the right address and update flag
    MallocMetadata* to_free = (MallocMetadata*)p - 1;
    to_free->is_free= true;
}

void* srealloc(void* oldp, size_t size){
    // check size and pointer
    if (size == 0 || size > MAXSIZE){
        return nullptr;
    }
    void* result;

    // if oldp == nullptr allocate size bytes and return
    if (oldp == nullptr){
        result = smalloc(size);
        if (result!=nullptr){
            return result;
        }
        return nullptr;
    }

    // if size < oldp.size() return oldp
    MallocMetadata* old_block = (MallocMetadata*)oldp-1;
    if (size <= old_block->size){
        return oldp;
    }
    // else need to get new allocation
    else{
        // result = smalloc()
        result = smalloc(size);
        // if allocation fail - return nullptr and dont sfree the oldp
        if  (result == nullptr){
            return nullptr;
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
    while (curr != nullptr){
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
    while (curr != nullptr){
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
    while (curr != nullptr){
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