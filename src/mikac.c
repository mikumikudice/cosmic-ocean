#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef char* str;

#define assert(cond) { if(!cond){\
    fprintf(stderr, "%s:%d:1 %s assertion failed",\
    __FILE__, __LINE__, #cond);\
    exit(1); } };

#define dynamic_array(type)\
    typedef struct { long cap; long size; type* items; } type ## _arr_t;\
    void type ## _arr_init(type ## _arr_t* v, long cap){\
        v->items = malloc(sizeof(type) * cap); v->size = 0; v->cap = cap;\
    };\
    void type ## _arr_append(type ## _arr_t* v, type i){\
        if(v->cap < v->size + 1){\
            v->cap *= 2; v->items = realloc(v->items, v->cap * sizeof(type));\
        };\
        v->items[v->size] = i; v->size += 1;\
    };

#define alloc(tp) _alloc(sizeof(tp))

void* _alloc(long size){
    void* ptr = malloc(size);
    memset(ptr, 0, size);
    return ptr;
};
