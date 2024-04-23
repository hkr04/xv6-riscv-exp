// Heap memory allocator
// Allocates bytes.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} hmem;

void
hinit()
{
  initlock(&hmem.lock, "hmem");
  freerange((void*)HEAPSTART, (void*)HEAPSTOP);
}

void
freerange(void *ba_start, void *ba_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)ba_start);
  for(; p < (char*)ba_end; p++)
    free(p);
}

void
free(void *ba)
{
    
}

void * 
malloc(size_t size)
{

}

