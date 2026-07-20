#ifndef OXYGEN_RT_H
#define OXYGEN_RT_H


#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#define OXYGEN_VERSION_MAJOR 0
#define OXYGEN_VERSION_MINOR 1
#define OXYGEN_VERSION_PATCH 0

#define OXYGEN_VERSION "0.1.0"

// HASH64

unsigned long long oxygen_hash64(long n) {
    unsigned long long hash= (n<<11) ^ 13 + n;
    return hash;
}

typedef struct string {
    char* data;
    unsigned int size;
    unsigned int capacity;
} string;



void oxygen_new_string(string* s, const char* str ) {
    s->size = strlen(str);
    s->capacity = s->size * 2+1;
    s->data = (char*)malloc(s->capacity+1);
    strcpy(s->data, str);
}

void oxygen_string_concat(string* s, const char* str) {
    unsigned int len = strlen(str);
    if(s->capacity<=(s->size+len+1)) {
        s->data = (char*)realloc(s->data, s->capacity+len+1);
        s->capacity += len+1;
    }
    strcat(s->data, str);
    s->size += len;
}

void oxygen_string_reserve(string* s, unsigned int size) {
    s->data = (char*)realloc(s->data, s->capacity+size);
    s->capacity += size;
}

void oxygen_string_destroy(string* s) {
    free(s->data);
}

// ptr_rt

void* oxygen_move(void **a) {
    void* temp = *a;
    *a = NULL;
    return temp;
}




// Vector

#define VEC(T) \
    typedef struct vec { \
        T* data; \
        unsigned long long size; \
        unsigned long long capacity; \
    } vec;


// Other

#define REF(x) (&(x))

#endif