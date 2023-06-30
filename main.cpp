#include <iostream>
#include <stdlib.h>

#define OUR_MALLOC
#ifdef OUR_MALLOC
#include "malloc_3.cpp"
//#define malloc smalloc
//#define free sfree
//#define calloc scalloc
//#define realloc srealloc
#endif

using namespace std;




int main() {

    init();

    MallocMetadata* ptr = block_lists[MAX_ORDER].popFirst();
    block_lists[MAX_ORDER].addToList(ptr);
    for (int i=0; i<MAX_ORDER-6; i++) {
        splitBlock(ptr);
    }
    MallocMetadata* ptr2 = block_lists[MAX_ORDER].popFirst();
    for (int i=0; i<MAX_ORDER-4; i++) {
        splitBlock(ptr2);
    }
    cout << "the chosen block:" << endl;
    printMetadata(findTheMatchBlock(10));

    print_all_lists();

    return 0;
}