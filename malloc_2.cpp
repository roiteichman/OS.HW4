//
// Created by teich on 26/06/2023.
//

//TODO: data structure for meta_data - theirs or like ATAM?
//TODO: global sorted LIST (by address) - our
//TODO: global counter_bytes
//TODO: global counter_total_blocks
//TODO: global counter_total_bytes

void* smalloc(size_t size){
    // check size

    // move on the "free_list"

    // search for fit block

    // if found - allocate it

        // remove from list

        // update the counters by the size of the block (not by size 10/1000 example)

        // return pointer to start

    // else if  - sbrk

        // return pointer

    // else (sbrk fail)

        // return null
}


void* scalloc(size_t num, size_t size){
    //like smalloc with ZERO

    // call smalloc

    // if not NULL memset 0

    // return
}

void sfree(void* p){
    // check if p is null or meta_data flag is free (p-size(meta_data))
        //return

    // else - free means add to list in sorted way update pointers next&prev update flag
}

void* srealloc(void* oldp, size_t size){
    // check size and pointer

    // if size < oldp.size()
        //return oldp

    // else
        // result = smalloc()
        // copy the content - with std::memmove()
        // sfree oldp
        // return

    //TODO - read instruction of Failure
}

size_t _num_free_blocks(){
    // return list.size();
}

size_t _num_free_bytes(){
    // return counter_bytes
}

size_t _num_allocated_blocks(){
    //return counter_total_blocks
}

size_t _num_allocated_bytes(){
    //return counter_total_bytes
}

size_t _num_meta_data_bytes(){
    //return counter_total_blocks*sizeof(meta_data)
}

size_t _size_meta_data(){
    //return sizeof(meta_data)
}