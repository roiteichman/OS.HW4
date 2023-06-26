//
// Created by teich on 26/06/2023.
//
//#include <iostream>
#include <cstdio>
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
    void* a = smalloc(0);
    void* b = smalloc(100000001);
    void* c = smalloc(-1);
    printf("a: %u\n", a);
    printf("b: %u\n", b);
    printf("c: %u\n", c);
    return 0;
}