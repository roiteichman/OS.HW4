//
// Created by teich on 28/06/2023.
//

//
// Created by teich on 26/06/2023.
//
#ifndef OS_HW4_MALLOC3_H
#define OS_HW4_MALLOC3_H

#include <cstring>
#include <unistd.h>
#include <iostream>
#include <assert.h>

#define DEBUG_PRINT

#define MAX_ORDER 10
#define MIN_SIZE ((size_t)128)
#define MAX_SIZE (SIZE_OF_ORDER(MAX_ORDER))
#define SIZE_OF_ORDER(x) ((size_t)(MIN_SIZE<<(x)))
#define INITIAL_BLOCK_NUM 32

int counter_total_blocks = 0;
bool system_initialized = false;
int global_magic = 0;


//TODO: address structure for meta_data - theirs or like ATAM?
struct MallocMetadata {
    int magic_num;
    int order;
    bool is_free;
    MallocMetadata* next;
    MallocMetadata* prev;
    MallocMetadata(int init_order = 0, bool init_is_free = false, MallocMetadata* init_next = nullptr, MallocMetadata* init_prev = nullptr);
    MallocMetadata(const MallocMetadata& other) = default;
};

//TODO: check metadata in every access

MallocMetadata::MallocMetadata(int init_order, bool init_is_free, MallocMetadata* init_next, MallocMetadata* init_prev) :
        magic_num(global_magic),
        order(init_order),
        is_free(init_is_free),
        next(init_next),
        prev(init_prev) {}


/*-------------------------------------------------
 * class BlockList:
 -------------------------------------------------*/


class BlockList {
public:
    BlockList();
    BlockList(const BlockList& other) = delete;
    const BlockList& operator=(const BlockList& other) = delete;

    void setOrder(int order);
    bool is_empty() const;
    void addToList (MallocMetadata* metadata);
    MallocMetadata* popFirst();
    void remove_block(MallocMetadata* block_to_remove);
#ifdef DEBUG_PRINT
    void print() const;
#endif
private:
    int m_order;
    int m_blocks_num;
    MallocMetadata* m_first;
};

BlockList::BlockList():
        m_order(0),
        m_blocks_num(0),
        m_first(nullptr)    {}



void BlockList::setOrder(int order) {
    m_order = order;
}

void BlockList::addToList(MallocMetadata* new_metadata) {
    assert(new_metadata->order == m_order);
    assert(new_metadata->is_free);

    // if the match location is the start of the list:
    if (m_first == nullptr || m_first > new_metadata) {
        if (m_first != nullptr) {
            assert(m_blocks_num > 0);
            m_first->prev = new_metadata;
        }
        else {
            assert(m_blocks_num==0);
        }
        new_metadata->next = m_first;
        new_metadata->prev = nullptr;
        m_first = new_metadata;
        m_blocks_num++;
        return;
    }

    // if the match location isn't the start:
    assert(m_blocks_num > 0);

    MallocMetadata* temp = m_first;
    // run the allocated blocks and find the right spot to enter the Malloc_new_block
    while (temp->next != nullptr && new_metadata > temp->next) {
        temp = temp->next;
    }

    new_metadata->next = temp->next;
    new_metadata->prev = temp;

    temp->next = new_metadata;
    if (new_metadata->next != nullptr) {
        new_metadata->next->prev = new_metadata;
    }

    m_blocks_num++;
    return;
}


MallocMetadata* BlockList::popFirst() {
    if (m_first == nullptr) {
        assert(m_blocks_num == 0);
        return nullptr;
    }
    assert(m_blocks_num > 0);

    MallocMetadata* result = m_first;
    m_first = m_first->next;
    if (m_first != nullptr) {
        assert(m_blocks_num > 1);
        m_first->prev = nullptr;
    }
    m_blocks_num--;

    result->next = nullptr;
    result->prev = nullptr;
    return result;
}


void BlockList::remove_block(MallocMetadata *block_to_remove) {
    assert(m_order == block_to_remove->order);
    assert(m_blocks_num > 0);
    assert(block_to_remove->is_free);

    if (block_to_remove->next != nullptr) {
        block_to_remove->next->prev = block_to_remove->prev;
    }

    if (block_to_remove->prev == nullptr) {
        assert(m_first == block_to_remove);
        m_first = block_to_remove->next;
    }
    else {
        assert(m_first != block_to_remove);
        block_to_remove->prev->next = block_to_remove->next;
    }

    block_to_remove->next = nullptr;
    block_to_remove->prev = nullptr;

    m_blocks_num--;
}

bool BlockList::is_empty() const {
    return (m_blocks_num == 0);
}

BlockList block_lists[MAX_ORDER+1];
// TODO: another linklist for big_allocations

/*----------------------------------
 initialize the system:
 ----------------------------------*/

// if we call init many times, it's OK
void init() {
    // check if the system is already initialized:
    if (system_initialized) return;
    system_initialized = true;

    // chose random magic number:
    global_magic = rand();

    // initial the array of lists:
    for (int order=0; order <= MAX_ORDER; order++) {
        block_lists[order].setOrder (order);
    }

    unsigned long curr_program_break = (unsigned long)sbrk(0);
    sbrk(MAX_SIZE - (curr_program_break % MAX_SIZE));
//    void* t = sbrk(0);
//    printf("%x\n", (unsigned long)t);
    void* start_ptr = sbrk(MAX_SIZE * INITIAL_BLOCK_NUM);
    void* block_ptr = start_ptr;

    MallocMetadata tmp(MAX_ORDER, true);

    for (int i=0; i<INITIAL_BLOCK_NUM; i++) {
        *(MallocMetadata*)block_ptr = tmp;
        block_lists[MAX_ORDER].addToList((MallocMetadata*)block_ptr);
        int* tmp_ptr = (int*)block_ptr;
        tmp_ptr += MAX_SIZE/sizeof(int);
        block_ptr = (void*)tmp_ptr;
    }
}


/*--------------------------------------
 split and merge blocks:
---------------------------------------*/


// in success - return pointer to the MallocMetadata of the new bloc
// insert the new block to the match list. set it free, and curr_block not free
// in failure (try to split order-0 block) - return null.
// important - this functions assume that curr_block will be allocated and the new will be free.
MallocMetadata* splitBlock (MallocMetadata* curr_block) {

    //curr_block->is_free = false;
    if (curr_block->order == 0) {
        return nullptr;
    }

    curr_block->order--;

    MallocMetadata new_block(curr_block->order, true);

    MallocMetadata* new_block_ptr = (MallocMetadata*)(curr_block + SIZE_OF_ORDER(curr_block->order)/sizeof(MallocMetadata));
    *new_block_ptr = new_block;

    block_lists[new_block.order].addToList(new_block_ptr);

    return new_block_ptr;
}



// the function merge the blocks iteratively until max size and remove the buddies from the lists, then insert the result to the match list.
// the function assume that cuur_block is not in list, but all the other free blocks are in lists.
MallocMetadata* mergeBlock (MallocMetadata* curr_block) {
    assert(curr_block->is_free);
    assert((unsigned long)curr_block % SIZE_OF_ORDER(curr_block->order) == 0);
    if (curr_block->order == MAX_ORDER) {
        return nullptr;
    }
    MallocMetadata *buddy = nullptr;

    while(curr_block->order < MAX_ORDER) {
        // if the buddy is in right:
        if ((unsigned long) curr_block % SIZE_OF_ORDER(curr_block->order + 1) == 0) {
            buddy = (MallocMetadata *) (curr_block + SIZE_OF_ORDER(curr_block->order)/sizeof(MallocMetadata));
            assert(buddy->order == curr_block->order);
            //if the current buddy is allocated:
            if (!buddy->is_free) {
                break;
            }
        }

        // if the buddy is in left:
        else {
            buddy = (MallocMetadata *) (curr_block - SIZE_OF_ORDER(curr_block->order));
            assert(buddy->order == curr_block->order);
            if (buddy->is_free) {
                curr_block = buddy;
            }
            //if the current buddy is allocated:
            else break;
        }
        // merge curr_block and buddy:
        block_lists[buddy->order].remove_block(buddy);
        curr_block->order++;
    }

    block_lists[curr_block->order].addToList(curr_block);
    return curr_block;
}

/*---------------------------------------
 * find matching blocks:
----------------------------------------*/

// return order that can contion the size. if it is larger than MAX_ORDER, return -1;
int findMatchOrder (size_t wanted_size) {
    wanted_size += sizeof(MallocMetadata);
    for (int order=0; order <= MAX_ORDER; order++) {
        if (wanted_size <= SIZE_OF_ORDER(order)) {
            return order;
        }
    }
    return -1;
}

MallocMetadata* findTheMatchBlock(size_t wanted_size) {
    int wanted_order = findMatchOrder(wanted_size);
    if (wanted_order == -1) {
        return nullptr;
    }
    MallocMetadata* match_block = nullptr;
    // find the block with the minimal order:
    for (int curr_order = wanted_order; curr_order <= MAX_ORDER && match_block == nullptr; curr_order++) {
        match_block = block_lists[curr_order].popFirst();
    }
    // if maching block not found:
    if (match_block == nullptr) {
        std::cout << "error: the memory is full!" << std::endl;
    }
    // set the block to the correct size:
    while (match_block->order > wanted_order) {
        splitBlock(match_block);
    }
    return match_block;
}

/*
void add_to_list(void* new_block){
    MallocMetadata* Malloc_new_block = (MallocMetadata*)new_block;

    // if first block - update the global list pointer + counter
    if(sorted_list==NULL){
        sorted_list = Malloc_new_block;
        counter_total_blocks++;
        return;
    }

    // create temp
    MallocMetadata* head = sorted_list;
    MallocMetadata* temp = head;

    // run the allocated blocks and find the right spot to enter the Malloc_new_block
    while (temp!=NULL) {
        // if need to be before temp
        if (Malloc_new_block < temp) {
            // save the temp_prev
            MallocMetadata *temp_prev = temp->prev;

            // update the prev to point on the Malloc_new_block
            if (temp_prev != NULL){
                temp_prev->next = Malloc_new_block;
            }
                // if temp_prev == NULL - means temp was the first elem
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
        else if (temp->next==NULL){
            temp->next = Malloc_new_block;
            Malloc_new_block->prev - temp;
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
    return NULL;
}


void* smalloc(size_t size){
    // check size
    if (size == 0 || size > MAXSIZE){
        return NULL;
    }

    void* new_block = find_free_block(size);
    if (new_block!=NULL){
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
        metadata.prev=NULL;
        metadata.next=NULL;

        // put the metadata struct in the place we allocate - convert the address from void* to metadata* and saved in new_block the metadata
        *(MallocMetadata*)new_block = metadata;

        // add to the allocations list
        add_to_list(new_block);

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
*/

#ifdef DEBUG_PRINT

/*---------------------------------------------
 * debug printings:
 ---------------------------------------------*/
using namespace std;

void printMetadata (MallocMetadata* m) {
    cout << "Malloc-Metadata:" << endl;
    cout << "location: " << m << endl;
    cout << "order: " << m->order << endl;
    (m->is_free) ? cout << "status: free\n" : cout << "status: allocated\n";
    cout << "next: " << m->next << endl;
    cout << "prev: " << m->prev << endl;
}

void BlockList::print() const {
    cout << "---------------------------------" << endl;
    cout << "oreder of list: " << m_order << endl;
    cout << "number of blocks in list: " << m_blocks_num << endl << endl;
    for (MallocMetadata* ptr = m_first; ptr != nullptr; ptr = ptr->next) {
        printMetadata(ptr);
        cout << endl;
    }
    cout << endl << "end of order " << m_order << " list" << endl;
    cout << "---------------------------------" << endl << endl;
}

void print_all_lists() {
    for (int i = 0; i<=MAX_ORDER; i++) {
        block_lists[i].print();
    }
}

#endif

#endif