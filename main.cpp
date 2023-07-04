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

#define LEN 90

template <class T>
void printArr(T* arr, int len) {
    for (int i = 0; i< len; i++) {
        cout << *(arr++) << " ";
    }
    cout << endl;
}

void print_values() {
    cout << "free blocks: " << _num_free_blocks() << endl;
    cout << "all blocks: " << _num_allocated_blocks() << endl;
    cout << "free bytes: " << _num_free_bytes() << endl;
    cout << "all bytes: " << _num_allocated_bytes() << endl;
}

int main() {
    void* ptr1 = smalloc(4);
    print_values();
    /*
    void* arr[LEN];
    for (int i=0 ; i<LEN; i++) {
        arr[i] = smalloc(i);
    }
    for (int i=0 ; i<LEN; i++) {
        sfree(arr[i]);
    }
     */
    return 0;
}
