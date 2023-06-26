//
// Created by teich on 26/06/2023.
//
#include <iostream>
#include <unistd.h>
#define MAXSIZE 100000000

void* smalloc(size_t size){
    if(size == 0 || size > MAXSIZE ){
        return NULL;
    }
    void* result = sbrk(size);
    if(result!=(void*)(-1)){
        return result;
    }
    return NULL;
}

int main(){
    unsigned int* a = smalloc(1);
    unsigned int* b = smalloc(1);
    printf("a: %d\n", a);
    printf("b: %d\n", b);
    return 0;
}