#ifndef OXYGEN_RT_H
#define OXYGEN_RT_H


#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#define OXYGEN_MAJOR_VER 0
#define OXYGEN_MINOR_VER 2
#define OXYGEN_PATCH_VER 0

#define OXYGEN_VERSION "0.2.0"

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
        s->data = (char*)realloc(s->data, s->capacity+len+1*2);
        s->capacity += len+1*2;
    }
    strcat(s->data, str);
    s->size += len;
}

void oxygen_string_reserve(string* s, unsigned int size) {
    if(s->capacity<size) s->data = (char*)realloc(s->data, size);
    s->capacity += size;
}

void oxygen_fatal_panic(const char* msg) {
    fprintf(stderr, "Oxygen-Runtime: \033[31merror\033[0m: %s", msg);
    
    fprintf(stderr, "Oxygen-Runtime: \e[1;94mnote\033[0m: sudden program exit can cause memory leak with autofreeing\n");
    exit(EXIT_FAILURE);
}

char oxygen_string_get(string* s, unsigned int indx) {
    if(indx>=s->size) {
        oxygen_fatal_panic("accessing string with out-of-bounds index\n");
    } else {
        return s->data[indx];
    }
}

void oxygen_string_set(string* s, unsigned int indx, char c) {
    if(indx>=s->size) {
        oxygen_fatal_panic("accessing string with out-of-bounds index\n");
    } else {
        s->data[indx] = c;
    }
}

void oxygen_string_append(string *s1, const char* s2, unsigned int len) {
    oxygen_string_reserve(s1, s1->size+len+1);

    memcpy(s1->data + s1->size, s2, len);

    s1->size += len;
    s1->data[s1->size]='\0';
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

const char* oxygen_fmt(const char* templ, const char* types, ...) {
    va_list args;
    va_start(args, types);
    unsigned int types_len = strlen(types);
    char* next;
    string dst;
    dst.data = (char*)malloc(256);
    dst.capacity = 256;
    dst.size = 0;
    dst.data[0] = '\0';
    unsigned long i = 0;
    const char *current = templ;
    string temp;
    temp.capacity = 256;
    temp.size = 0;
    temp.data = (char*)calloc(256, sizeof(char));
    while((next=(char*)strstr(current, "{}"))!=NULL) {
        char ct = types[i++];
        unsigned int len = next - current;
        oxygen_string_append(&dst, current, len);
        switch(ct) {
            case 's': {
                const char *s_val = va_arg(args, const char*);
                snprintf(temp.data, temp.capacity, "%s", s_val);
                break;
            }
            case 'i': {
                const long long i_val = va_arg(args, long long);
                snprintf(temp.data, temp.capacity, "%lld", i_val);
                break;
            }
            case 'u': {
                const unsigned long long u_val = va_arg(args, unsigned long long);
                snprintf(temp.data, temp.capacity, "%llu", u_val);
                break;
            }
            case 'f': {
                const double f_val = va_arg(args, double);
                snprintf(temp.data, temp.capacity, "%f", f_val);
                break;
            }
            case 'b': {
                const bool b_val = va_arg(args, int);
                if(b_val == 0) snprintf(temp.data, temp.capacity, "false");
                else snprintf(temp.data, temp.capacity, "true");
                break;
            }
            default: {
                snprintf(temp.data, temp.capacity, "{?}");
                break;
            }
        }
        oxygen_string_concat(&dst, temp.data);
        current = next + 2;
    }
    oxygen_string_concat(&dst, current);
    va_end(args);

    oxygen_string_destroy(&temp);
    return dst.data;
}

#define REF(x) (&(x))

#endif