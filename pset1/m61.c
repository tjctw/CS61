/*
    CS61 Problem Set1. Debugging memory allocation.

    The task is to write a debugging malloc library that helps track and debug memory usage, 
    and that catches memory errors in our handout programs (and other programs).

    Tim Gabets <gabets@g.harvard.edu>
    September 2013
*/

#define M61_DISABLE 1
#include "m61.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>

// memory status:
#define INACTIVE 0
#define ACTIVE 1
#define FAILED 2

// memory free return statuses:
#define SUCCESS 1
#define FAIL 0
#define INVLDFREE -1
#define NOTINHEAP -2
#define NOTALLOC -3

struct list 
{
    void*           address;    // pointer to allocated memory 
    int             status;     // 0 is inactive, 1 is active, 2 is failed
    size_t          size;       // size of allocated memory
    struct list*    prev;       // previous item in the list
    struct list*    next;       // next item in the list
};

struct list* head = NULL;

void *m61_malloc(size_t sz, const char *file, int line) 
{
    (void) file, (void) line;   // avoid uninitialized variable warnings

    void* ptr = malloc(sz);

    if(ptr != NULL)
        m61_add2list(ptr, sz, ACTIVE);
    else
        m61_add2list(ptr, sz, FAILED);

    return ptr;
}

void m61_free(void *ptr, const char *file, int line) 
{
    (void) file, (void) line;   // avoid uninitialized variable warnings

    int rmstatus = m61_removefromlist(ptr);

    if(rmstatus == SUCCESS)
        free(ptr);
    else    
    {
        if(rmstatus == NOTINHEAP) 
            fprintf( stderr, "MEMORY BUG: %s:%d: invalid free of pointer %p, not in heap\n", file, line, ptr);
    
        if(rmstatus == INVLDFREE)
            fprintf(stderr, "MEMORY BUG: %s:%d: invalid free of pointer %p\n", file, line, ptr);
    
        if(rmstatus == NOTALLOC)
            fprintf(stderr, "MEMORY BUG: %s:%d: invalid free of pointer %p, not allocated\n", file, line, ptr);
    }

}

void *m61_realloc(void *ptr, size_t sz, const char *file, int line) 
{
    void *new_ptr = NULL;
    
    if (sz)
        new_ptr = m61_malloc(sz, file, line);
    else
    {
        // reallocating zero bytes of memory? Okay!
        m61_free(ptr, file, line);
        return NULL;
    }

    if(head != NULL)
    {
        struct list* temp = head;
        while(temp -> next != NULL)
        {
            if(temp -> address == ptr)
            {
                memcpy(new_ptr, ptr, temp -> size);
                m61_free(ptr, file, line);
            }


            temp = temp -> next;
        }
    }   
    return new_ptr;
}

void *m61_calloc(size_t nmemb, size_t sz, const char *file, int line) 
{
    // Your code here (to fix test010).
    void *ptr = m61_malloc(nmemb * sz, file, line);
    if (ptr)
        memset(ptr, 0, nmemb * sz);
    return ptr;
}

void m61_getstatistics(struct m61_statistics* stats)
{
    
    stats -> nactive = 0;        
    stats -> active_size = 0;    
    stats -> ntotal = 0;         
    stats -> total_size = 0;     
    stats -> nfail = 0;          
    stats -> fail_size = 0;
    

    if(head != NULL)
    {
        struct list* temp = head;

        do{
            /* A simple remainder:
            struct m61_statistics {
                unsigned long long nactive;         // # active allocations
                unsigned long long active_size;     // # bytes in active allocations
                unsigned long long ntotal;          // # total allocations
                unsigned long long total_size;      // # bytes in total allocations
                unsigned long long nfail;           // # failed allocation attempts
                unsigned long long fail_size;       // # bytes in failed alloc attempts
                };
            */
            if(temp -> status == ACTIVE)
            {
                stats -> nactive++;    
                stats -> total_size += temp -> size;
                stats -> active_size += temp -> size;    
                stats -> ntotal++;
            }

            if(temp -> status == INACTIVE)
            {
                stats -> total_size += temp -> size;
                stats -> ntotal++;
            }

            if(temp -> status == FAILED)
            {
                stats -> nfail++;
                stats -> fail_size += temp -> size;
            }
            
            temp = temp -> next;

        }while(temp != NULL);
    }    
}

void m61_printstatistics(void) 
{
    struct m61_statistics stats;
    m61_getstatistics(&stats);

    printf("malloc count: active %10llu   total %10llu   fail %10llu\n",
           stats.nactive, stats.ntotal, stats.nfail);
    printf("malloc size:  active %10llu   total %10llu   fail %10llu\n",
           stats.active_size, stats.total_size, stats.fail_size);
}

void m61_printleakreport(void) 
{
    // Your code here.
}

// adding items to the list
int m61_add2list(void* ptr, size_t sz, int status)
 {
   
    if(head != NULL)
    {
        struct list* temp = head;
        
        while(temp -> next != NULL)
            temp = temp -> next;
      
        // at this point temp is tail (the last item in the list)
        struct list* tail = malloc( sizeof(struct list) );

        temp -> next = tail;
        tail -> address = ptr;
        tail -> size = sz;
        tail -> status = status;
        tail -> prev = temp;
        tail -> next = NULL;       

        return SUCCESS;   // success
    }
    else
    {
        head = malloc( sizeof(struct list) );
        head -> address = ptr;
        head -> size = sz;
        head -> status = status;
        head -> next = NULL;
        head -> prev = NULL;

        return SUCCESS; // success
    }       
}

// we don't actualy removing items from list, just marking them as INACTIVE
/*
 * FAILED and INACTIVE items will be removed from the list.    

*/
int m61_removefromlist(void* ptr)
{
    // running through the linked list to find needed pointer
    if(head != NULL)
    {
        struct list* temp = head;
        while(temp -> next != NULL)
        {
            if(temp -> address == ptr)
                break;
            temp = temp -> next;
        }
    
        // at this point we either at the tail or at the needed item:
        if(temp -> address == ptr)
        {
            if(temp -> status == ACTIVE)
            {
                temp -> status = INACTIVE;
                temp -> address = NULL;
                return SUCCESS;
            }
            else    // memory was already freed
            {
                return INVLDFREE;  // MEMORY BUG???: invalid free of pointer ???
            }
        }else
        {
            return NOTALLOC;
        }
        
    }
    else // no memory was allocated 
    {
        // TODO: checking the type of the poiner - is it from heap or from stack.
        return NOTINHEAP;     // MEMORY BUG???: invalid free of pointer ???, not allocated
    }
}