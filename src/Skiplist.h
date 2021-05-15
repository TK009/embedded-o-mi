/*  A simple C skip list implementation mapping string keys to string values.
 *
 *  Designed with simplicity and minimalism in mind, this implementation uses
 *  fixed-size pillar arrays.
 *
 *  I wrote this because skip lists are my favorite data structure. They are
 *  relatively easy to write and understand (in contrast with certain types of
 *  self-balancing binary trees), use no "magic" hash functions, and still
 *  manage to provide expected O(log n) insertion, search, and deletion.
 *
 * Copyright (C) 2013 Troy Deck
 *  2021 Modified by Tuomas Keyriläinen
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */
#ifndef SLIST
#define SLIST   1

// This defines the maximum number of next pointers that can be stored in a
// single node. For good performance, a good rule of thumb is to set this to
// lg(N), where:
//      lg() is the base-2 logarithm function
//      N is the maximum number of entries in your list
//  If you set this to 1, you will get a slightly slower sorted linked list 
//  implementation.
#define MAX_SKIPLIST_HEIGHT     8

#include "utils.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SkiplistEntry {  
    void * key;
    void * value;
    int height;
    struct SkiplistEntry * next[MAX_SKIPLIST_HEIGHT];
} SkiplistEntry;

typedef struct Skiplist {
    SkiplistEntry *head;
    compareFunc compare;
    Allocator* alloc;
} Skiplist;

Skiplist * Skiplist_init(Skiplist* sl, compareFunc compare, Allocator* a);
void Skiplist_destroy(Skiplist* slist);
void* Skiplist_get(Skiplist* slist, const void* key); // return the value
SkiplistEntry* Skiplist_findP(Skiplist* slist, const void* key, SkiplistEntry * prev[MAX_SKIPLIST_HEIGHT]); // return the value
SkiplistEntry* Skiplist_find(Skiplist* slist, const void *key);

void Skiplist_del(Skiplist* slist, SkiplistEntry *elem, SkiplistEntry * prev[MAX_SKIPLIST_HEIGHT]);

// return the replaced value or NULL if new; existing keys are not replaced
Pair Skiplist_set(Skiplist* slist, void* key, void* value); 

Pair Skiplist_pop(Skiplist* slist, const void* key); // remove and return the value


#ifdef __cplusplus
}
#endif
#endif
