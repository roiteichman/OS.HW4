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
    void* a = smalloc(1);
    void* b = smalloc(1);
    printf("a: %ld\n", a);
    printf("b: %ld\n", b);
    return 0;
}