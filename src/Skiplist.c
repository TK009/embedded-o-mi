/*
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
#include "Skiplist.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

bool seeded = false;


// Returns a random number in the range [1, max] following the geometric 
// distribution.
static int grand (int max) {
    int result = 1;

    while (result < max && (rand() > RAND_MAX / 2)) {
        ++ result;
    }

    return result;
}

// Keey original Skiplist* if dynamically allocated, in order to free it if this returns NULL
Skiplist * Skiplist_init(Skiplist* sl, compareFunc compare, Allocator* a){
    if (sl) {
        // Seed the random number generator if we haven't yet
        if (!seeded) {
            srand((uint) time(NULL));
            seeded = true;
        }


        // Construct and return the head sentinel
        SkiplistEntry * head = a->calloc(1, sizeof(SkiplistEntry));
        if (!head) return NULL;
        head->height = MAX_SKIPLIST_HEIGHT;

        *sl = (Skiplist){.head = head, .compare = compare, .alloc = a };
    }
    return sl;
}

void Skiplist_destroy(Skiplist* slist){
    SkiplistEntry * current_entry = slist->head;
    SkiplistEntry * next_entry = NULL;
    while (current_entry) {
        next_entry = current_entry->next[0];
        slist->alloc->free(current_entry);
        current_entry = next_entry;
    }
}

// return the associated value, or NULL if the key was not found.
void* Skiplist_get(Skiplist* slist, const void* key){
    SkiplistEntry* res = Skiplist_find(slist, key);
    if (res) return res->value;
    return NULL;
}

SkiplistEntry* Skiplist_find(Skiplist* slist, const void *key) {
    SkiplistEntry * prev[MAX_SKIPLIST_HEIGHT];
    return Skiplist_findP(slist, key, prev);
}
// Get with previous link information
SkiplistEntry* Skiplist_findP(Skiplist* slist, const void* key, SkiplistEntry * prev[MAX_SKIPLIST_HEIGHT]) {
    SkiplistEntry * curr = slist->head;
    int level = curr->height - 1;
    
    while (curr != NULL && level >= 0) {
        prev[level] = curr;
        if (curr->next[level] == NULL) { // End of this level
            -- level;
        } else {
            int cmp = slist->compare(curr->next[level]->key, key);
            if (cmp == 0) { // Found
                for (int i = level;i >= 0; --i)
                    prev[i] = curr;
                return (curr->next[level]); 
            } else if (cmp > 0) { // Drop down a level 
                -- level;
            } else { // Keep going at this level
                curr = curr->next[level];
            }
        }
    }
    return NULL; // not found
}
void Skiplist_del(Skiplist* slist, SkiplistEntry *condemned, SkiplistEntry * prev[MAX_SKIPLIST_HEIGHT]) {
    // Remove the condemned node from the chain
    int i;
    for (i = condemned->height - 1; i >= 0; -- i) {
        prev[i]->next[i] = condemned->next[i];
    }
    slist->alloc->free(condemned);
}

// return the (current key, replaced value), (NULL, 1) if malloc fails, or (NULL, NULL) if new; existing keys are not replaced but their values are
Pair Skiplist_set(Skiplist* slist, void* key, void* value) {
    SkiplistEntry * prev[MAX_SKIPLIST_HEIGHT];
    SkiplistEntry * curr = slist->head;
    int level = curr->height - 1;

    // Find the position where the key is expected
    while (curr != NULL && level >= 0) {
        prev[level] = curr;
        if (curr->next[level] == NULL) { //end
            -- level;
        } else {
            int cmp = slist->compare(curr->next[level]->key, key);
            if (cmp == 0) { // Found
                void* old = curr->next[level]->value;
                curr->next[level]->value = (value);
                return (Pair){curr->next[level]->key, old};
            } else if (cmp > 0) { // Drop down a level 
                -- level;
            } else { // Keep going at this level
                curr = curr->next[level];
            }
        }
    }

    // Didn't find it, we need to insert a new entry
    SkiplistEntry * new_entry = slist->alloc->calloc(1,sizeof(SkiplistEntry));
    if (!new_entry) return (Pair){NULL, (void*)1}; // XXX: Special return value to indicate malloc fail
    new_entry->height = grand(slist->head->height);
    new_entry->key = key;
    new_entry->value = value;
    int i;
    // Null out pointers above height
    for (i = MAX_SKIPLIST_HEIGHT - 1; i > new_entry->height; -- i) { 
        new_entry->next[i] = NULL;
    }
    // Tie in other pointers
    for (i = new_entry->height - 1; i >= 0; -- i) {
        new_entry->next[i] = prev[i]->next[i];
        prev[i]->next[i] = new_entry;
    }
    return (Pair){ NULL, NULL };
}


Pair Skiplist_pop(Skiplist* slist, const void* key){ // remove and return the value
    SkiplistEntry * prev[MAX_SKIPLIST_HEIGHT];
    SkiplistEntry * elem = Skiplist_findP(slist, key, prev);
    //SkiplistEntry * curr = slist->head;
    //int level = curr->height - 1;

    //// Find the list node just before the condemned node at every
    //// level of the chain
    //int cmp = 1;
    //while (curr != NULL && level >= 0) {
    //    prev[level] = curr;
    //    if (curr->next[level] == NULL) { // End
    //        -- level;
    //    } else {
    //        cmp = slist->compare(curr->next[level]->key, key);
    //        if (cmp >= 0) { // Drop down a level 
    //            -- level;
    //        } else { // Keep going at this level
    //            curr = curr->next[level];
    //        }
    //    }
    //}

    // We found the match we want, and it's in the next pointer
    //if (curr && !cmp) { 
    if (elem) { 
        SkiplistEntry * condemned = elem; //curr->next[0];
        // Remove the condemned node from the chain
        int i;
        for (i = condemned->height - 1; i >= 0; -- i) {
          prev[i]->next[i] = condemned->next[i];
        }
        // Free it
        Pair ret = {condemned->key, condemned->value};
        slist->alloc->free(condemned);
        condemned = NULL;
        return ret;
    }
    return (Pair){NULL, NULL};
}
