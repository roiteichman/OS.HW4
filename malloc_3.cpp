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
#define NDEBUG
#include <assert.h>
#include <sys/mman.h>
#include <cstdlib>

//#define DEBUG_PRINT

#define SIZE_OF_ORDER(x) ((size_t)(MIN_SIZE<<(x)))

#define MAX_ORDER 10
#define MIN_SIZE ((size_t)128)
#define MAX_SIZE (SIZE_OF_ORDER(MAX_ORDER))
#define INITIAL_BLOCK_NUM 32
#define SIZE_LIMITATION 100000000
#define PAGE_SIZE 4096


int counter_total_blocks_used = 0;
size_t counter_total_bytes_used = 0;
bool system_initialized = false;
int global_magic = 0;


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


/*---------------------------------------
* safety & security check
----------------------------------------*/

void safety(MallocMetadata* block){
    if (block == nullptr) return;
    if (block->magic_num != global_magic) {
        exit(0xdeadbeef);
    }
}

/*-------------------------------------------------
 * class BlockList:
 -------------------------------------------------*/


class BlockList {
public:
    BlockList();
    BlockList(const BlockList& other) = delete;
    const BlockList& operator=(const BlockList& other) = delete;

    void setOrder(int order);
    int len() const;
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

    safety(m_first);

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
        safety(temp);
    }

    new_metadata->next = temp->next;
    new_metadata->prev = temp;

    temp->next = new_metadata;
    if (new_metadata->next != nullptr) {
        safety(new_metadata->next);
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
    safety(result);
    m_first = m_first->next;
    safety(m_first);

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
        safety(block_to_remove->next);
        block_to_remove->next->prev = block_to_remove->prev;
    }

    if (block_to_remove->prev == nullptr) {
        assert(m_first == block_to_remove);
        m_first = block_to_remove->next;
    }
    else {
        safety(block_to_remove->prev);
        assert(m_first != block_to_remove);
        block_to_remove->prev->next = block_to_remove->next;
    }

    block_to_remove->next = nullptr;
    block_to_remove->prev = nullptr;

    m_blocks_num--;
}

int BlockList::len() const {
    return m_blocks_num;
}

BlockList block_lists[MAX_ORDER+1];
/*-----------------------------------
 *  LINK LIST for the big_allocations
 ------------------------------------*/
// TODO: another linklist for big_allocations

class List{
private:
    int m_list_size;
    MallocMetadata* m_first;
public:
    List();
    List(const List& other) = delete;
    const List& operator=(const List& other) = delete;

    void addToList (MallocMetadata* new_block);
    void remove_block(MallocMetadata* block_to_remove);
    int get_len() const;
    size_t allocated_bytes() const;
};

List::List(): m_list_size(0), m_first(nullptr) {}

void List::addToList(MallocMetadata* new_block) {
    MallocMetadata* prev_head = m_first;
    safety(prev_head);
    // add the new big block in the head of the list
    m_first = new_block;
    // update the next block of it to be the prev head of the list
    new_block->next = prev_head;
    // if the list wasnt empty before, update the prev of the last head to be the new head
    if (prev_head!= nullptr){
        assert(prev_head->prev == nullptr);
        prev_head->prev = new_block;
    }
    m_list_size++;
}

void List::remove_block(MallocMetadata *block_to_remove) {
    MallocMetadata* curr = m_first;
    MallocMetadata* prev_of_removed = nullptr;
    while (curr != nullptr){
        safety(curr);
        // found
        if (curr == block_to_remove){
            prev_of_removed = block_to_remove->prev;

            // update the block_to_remove->prev->next = block_to_remove->next
            if (prev_of_removed != nullptr){
                prev_of_removed->next = block_to_remove->next;
            }
            else {
                // curr == m_first
                assert(m_first == curr);
                m_first = curr->next;
            }

            // update the block_to_remove->next->prev = block_to_remove->prev
            if (block_to_remove->next != nullptr){
                safety(block_to_remove->next);
                block_to_remove->next->prev = prev_of_removed;
            }
            break;
        }

        // continue to the next block
        curr=curr->next;
    }
    m_list_size--;
}

int List::get_len() const {
    return m_list_size;
}

size_t List::allocated_bytes() const {
    size_t result = 0;
    for (MallocMetadata* ptr = m_first; ptr != nullptr; ptr = ptr->next) {
        safety(ptr);
        result += ptr->order;
    }
    return result;
}

List big_block_list;


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

    unsigned long long curr_program_break = (unsigned long long)sbrk(0);
    sbrk(MAX_SIZE - (curr_program_break % MAX_SIZE));

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


bool merge (MallocMetadata** curr_block) {

    MallocMetadata *buddy = nullptr;
    // if the buddy is in right:
    if ((unsigned long long) (*curr_block) % SIZE_OF_ORDER((*curr_block)->order + 1) == 0) {
        buddy = (*curr_block) + SIZE_OF_ORDER((*curr_block)->order)/sizeof(MallocMetadata);
        safety(buddy);
        assert(buddy->order <= (*curr_block)->order);
        //if the current buddy is allocated:
        if (buddy->order < (*curr_block)->order || !buddy->is_free) {
            return false;
        }
    }

        // if the buddy is in left:
    else {
        buddy = (*curr_block) - SIZE_OF_ORDER((*curr_block)->order)/sizeof(MallocMetadata);
        safety(buddy);
        assert(buddy->order <= (*curr_block)->order);
        if (buddy->order < (*curr_block)->order || buddy->is_free) {
            (*curr_block) = buddy;
        }
            //if the current buddy is allocated:
        else return false;
    }
    // merge curr_block and buddy:
    block_lists[buddy->order].remove_block(buddy);
    (*curr_block)->order++;
    return true;
}


// the function merge the blocks iteratively until max size and remove the buddies from the lists, then insert the result to the match list.
// the function assume that cuur_block is not in list, but all the other free blocks are in lists.
MallocMetadata* mergeToList (MallocMetadata* curr_block) {
    assert(curr_block->is_free);
    assert((unsigned long long)curr_block % SIZE_OF_ORDER(curr_block->order) == 0);
//    if (curr_block->order == MAX_ORDER) {
//        return nullptr;
//    }

    while(curr_block->order < MAX_ORDER) {
        if (merge(&curr_block) == false) break;
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

MallocMetadata* findTheMatchBlock(int wanted_order) {

    MallocMetadata* match_block = nullptr;
    // find the block with the minimal order:
    for (int curr_order = wanted_order; curr_order <= MAX_ORDER && match_block == nullptr; curr_order++) {
        match_block = block_lists[curr_order].popFirst();
    }
    // if matching block not found:
    if (match_block == nullptr) {
        //std::cout << "error: the memory is full!" << std::endl;
        return nullptr;
    }
    // set the block to the correct size:
    while (match_block->order > wanted_order) {
        splitBlock(match_block);
    }
    return match_block;
}

/*---------------------------------------
 * large allocations:
----------------------------------------*/

MallocMetadata* allocate_big_block(size_t wanted_size){
    //size_t real_size = ((wanted_size+sizeof(MallocMetadata)-1)/PAGE_SIZE)+PAGE_SIZE;

    void* result = mmap(nullptr, wanted_size+sizeof(MallocMetadata), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (result == (void*)(-1)){
        perror("mmap fail: ");
    }
    else {
        MallocMetadata tmp(wanted_size);
        *(MallocMetadata*)result = tmp;
        // entered to list
        big_block_list.addToList((MallocMetadata*)result);
    }
    return (MallocMetadata*)result;
}

int free_big_block(MallocMetadata* block_to_delete){
    big_block_list.remove_block(block_to_delete);
    int result = munmap((void*)block_to_delete, (size_t)((block_to_delete->order)+sizeof(MallocMetadata)));
    if (result == -1){
        big_block_list.addToList(block_to_delete);
        perror("munmap fail: ");
    }
    return result;
}

/*---------------------------------------
 * implement the relevant functions:
----------------------------------------*/

void* smalloc(size_t size){
    init();
    // check size
    if (size == 0 || size > SIZE_LIMITATION){
        return nullptr;
    }
    int wanted_order = findMatchOrder(size);
    MallocMetadata* new_block = nullptr;
    // big size:
    if (wanted_order == -1) {
        new_block = allocate_big_block(size);
        /*if (new_block == (void*)(-1)){
            return nullptr;
        }*/
        return (void*)(new_block+1);
    }

    // regular size:
    new_block = findTheMatchBlock(wanted_order);
    // if the memory is full:
    if (new_block == nullptr) {
        return nullptr;
    }
    assert(new_block->is_free);
    assert(new_block->order == wanted_order);
    new_block->is_free = false;
    assert(counter_total_blocks_used >= 0);
    assert(counter_total_bytes_used >= 0);
    counter_total_blocks_used++;
    counter_total_bytes_used += ((SIZE_OF_ORDER(new_block->order)-sizeof(MallocMetadata)));
    return (void*) (new_block+1);
}


void* scalloc(size_t num, size_t size){
    init();
    // call smalloc
    void* new_block = smalloc(num * size);
    if (new_block == nullptr){
        return nullptr;
    }
    // if not NULL - find size to put 0:
    MallocMetadata* block_data = (MallocMetadata*)new_block -1;
    assert(block_data->is_free == false);
    assert(block_data->magic_num == global_magic);

    size_t size_to_zero = (block_data->order <= MAX_ORDER) ? (SIZE_OF_ORDER(block_data->order)-sizeof(MallocMetadata)) : block_data->order;
    // put 0
    std::memset(new_block, 0, size_to_zero);

    return new_block;
}

void sfree(void* p){
    init();
    // check if p is null or meta_data flag is free (p-size(meta_data))
    if (p == nullptr){
        return;
    }
    // else - free means get the right address and update flag
    MallocMetadata* to_free = (MallocMetadata*)p - 1;
    safety(to_free);

    if (to_free->is_free){
        // dont need to free p again - so return
        return;
    }

    to_free->is_free= true;

    // regular block:
    if (to_free->order <= MAX_ORDER) {
        counter_total_blocks_used--;
        assert(counter_total_blocks_used >= 0);
        counter_total_bytes_used -= ((SIZE_OF_ORDER(to_free->order)-sizeof(MallocMetadata)));
        assert(counter_total_bytes_used >= 0);
        mergeToList(to_free);
    }
    // big block:
    else {
        free_big_block(to_free);
    }
}

//TODO: implement from here

void* big_block_realloc(MallocMetadata* old_block, size_t size){
    void* new_block = nullptr;

    // if size is equal so return the current block
    if ((size_t)old_block->order == size){
        return old_block;
    }
    // else size is different and needed to mmap new block and munmap the previous one
    else{
        new_block = smalloc(size);
    }
    if (new_block != nullptr) {
        std::memmove(new_block,(void*)(old_block+1), old_block->order);
        sfree((void*)(old_block+1));
    }
    return new_block;
}


void* srealloc(void* oldp, size_t size){
    init();

    MallocMetadata* old_block = (MallocMetadata*)oldp-1;


    safety(old_block);

    // check size and pointer
    if (size == 0 || size > SIZE_LIMITATION){
        return nullptr;
    }

    int wanted_order = findMatchOrder(size);
    // if big block:
    if (wanted_order == -1) {
        return big_block_realloc(old_block, size);
    }
    // else regular block
    size_t src_size = SIZE_OF_ORDER(old_block->order)-sizeof(MallocMetadata);
    // saved bytes used
    size_t old_bytes_used = SIZE_OF_ORDER(old_block->order);
    // try to merge until the wanted size:
    while (old_block->order < wanted_order) {
        if (merge(&old_block) == false) break;
    }
    // if the block is big enough:
    if (old_block->order >= wanted_order) {
        // update counter of bytes in used
        counter_total_bytes_used += (SIZE_OF_ORDER(old_block->order)-old_bytes_used);
        std::memmove((void*)(old_block+1), oldp, src_size);
        return (void*)(old_block+1);
    }
    // alloc another block:
    void* new_block = nullptr;
    new_block = smalloc(size);

    if (new_block != nullptr) {
        std::memmove(new_block, oldp, src_size);
        sfree((void*)(old_block+1));
    }
    return new_block;
}

/*----------------------------------
 * num of used blocks and bytes:
 ----------------------------------*/


size_t _num_free_blocks(){
    if(!system_initialized){
        return 0;
    }
    // sum len of block_list for every entry

    // init counter
    size_t counter = 0;

    // get the number of small free blocks
    for (int i = 0; i <= MAX_ORDER; ++i) {
        counter += block_lists[i].len();
    }

    // mind that big blocks are not freed

    // return counter
    return counter;
}

size_t _num_free_bytes(){
    if(!system_initialized){
        return 0;
    }
    // like free_block but instead of return the len*(sizeof(order)-metadata)

    // init total_free_space
    size_t total_free_space = 0;

    for (int i = 0; i <= MAX_ORDER; ++i) {
        total_free_space += (block_lists[i].len())*(SIZE_OF_ORDER(i)-sizeof(MallocMetadata));
    }

    // mind that big blocks are not freed

    return total_free_space;
}

size_t _num_allocated_blocks(){
    if(!system_initialized){
        return 0;
    }
    return counter_total_blocks_used+ _num_free_blocks() +big_block_list.get_len();
    }

size_t _num_allocated_bytes(){
    if(!system_initialized){
        return 0;
    }
    return counter_total_bytes_used+big_block_list.allocated_bytes()+_num_free_bytes();
}

size_t _num_meta_data_bytes(){
    if(!system_initialized){
        return 0;
    }
    return _num_allocated_blocks()*sizeof(MallocMetadata);
}

size_t _size_meta_data(){
    return sizeof(MallocMetadata);
}


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