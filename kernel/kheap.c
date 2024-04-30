// Heap memory allocator
// Allocates bytes.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

#include <stdbool.h>
#include <stddef.h>

// check if an byte address is the start of a block to be allocated
bool bitmap[HEAP_SIZE]; 

struct run {
  struct run *prev, *next;
  size_t size; // including the pointer
};

struct {
  struct spinlock lock;
  struct run *freelist;
} hmem;

void
show_heap()
{
  acquire(&hmem.lock);
  int total_free_blocks = 0;
  size_t total_free_space = 0;
  size_t max_block_size = 0;
  size_t min_block_size = HEAP_SIZE;
  printf("============ HEAP INFO ============\n");
  struct run *cur = hmem.freelist;
  while (cur != NULL) {
    printf("Block at %d, size of %d bytes\n", (uint64)cur - HEAPSTART, cur->size);
    total_free_blocks++;
    total_free_space += cur->size;
    if (cur->size > max_block_size) max_block_size = cur->size;
    if (cur->size < min_block_size) min_block_size = cur->size;
    // if (cur->next)
    //   printf("next: %d\n", (uint64)cur->next-HEAPSTART);
    // else
    //   printf("next: NULL\n");
    cur = cur->next;
  }
  printf("Total free block(s): %d\n", total_free_blocks);
  printf("Total free space: %d bytes\n", total_free_space);
  printf("Max free block size: %d bytes\n", max_block_size);
  printf("Min free block size: %d bytes\n", min_block_size);
  printf("===================================\n");
  release(&hmem.lock);
}

void
set_bitmap(struct run *block, bool flag)
{
  bitmap[(uint64)block - HEAPSTART] = flag;
}

bool
get_bitmap(struct run *block)
{
  return bitmap[(uint64)block - HEAPSTART];
}

void
delete_from_freelist(struct run *block)
{
  if (block->prev)
    block->prev->next = block->next;
  else
    hmem.freelist = block->next;
  if (block->next)
    block->next->prev = block->prev;
  set_bitmap(block, false);
}

void
hinit()
{
  initlock(&hmem.lock, "hmem");
  memset(bitmap, 0, sizeof(bitmap));
  hmem.freelist = (struct run*)HEAPSTART;
  hmem.freelist->size = HEAP_SIZE;
  hmem.freelist->prev = NULL;
  hmem.freelist->next = NULL;
  // show_heap();
  set_bitmap(hmem.freelist, true); // only one block is avalible
}

struct run *
buddy_of(struct run *block) 
{
  return (struct run *)((uint64)block ^ block->size);
}

void
try_merge(struct run *block)
{
  if ((uint64)block < HEAPSTART || (uint64)block >= HEAPSTOP)
    return;
  
  struct run *buddy;

  while (block->size < HEAP_SIZE) {
    buddy = buddy_of(block);
    if ((uint64)buddy < HEAPSTART || (uint64)buddy >= HEAPSTOP)
      break;
    if (!get_bitmap(buddy) || buddy->size != block->size)
      break;
    // printf("current block: %d, buddy: %d, size: %d\n", (uint64)block-HEAPSTART, (uint64)buddy-HEAPSTART, block->size);
    // printf("prev: %d, next: %d\n", (uint64)block->prev-HEAPSTART, (uint64)block->next-HEAPSTART);
    if (block < buddy) {
      block->size <<= 1;
      delete_from_freelist(buddy);
    } else {
      buddy->size <<= 1;
      delete_from_freelist(block);
      block = buddy;
    }
  }
}

void
free(void *ba)
{
  struct run *r = (struct run*)((uint64)ba - sizeof(struct run));

  if((uint64)r < HEAPSTART || (uint64)r >= HEAPSTOP)
    panic("free: out of range");

  // Avoid double free
  if(get_bitmap(r)) 
    return;

  // printf("free %d bytes at address %d(%d bytes for header)\n", r->size, (uint64)r - HEAPSTART, sizeof(struct run));

  // Fill with junk.
  memset(ba, 1, r->size - sizeof(struct run));

  acquire(&hmem.lock);
  // insert
  if (!hmem.freelist)
    hmem.freelist = r, r->prev = NULL, r->next = NULL;
  else {
    r->next = hmem.freelist;
    hmem.freelist->prev = r;
    // update head pointer
    hmem.freelist = r;
    r->prev = NULL;
    set_bitmap(r, true);
    // try merging with buddy
    try_merge(r);
  }
  release(&hmem.lock);
}

struct run *
find_free_block(size_t size)
{
  struct run *r = hmem.freelist;
  while (r != NULL) {
    // printf("address: %d, size: %d\n", (uint64)r - HEAPSTART, r->size);
    if (r->size == size)
      break;
    else if (r->size > size) { 
      if ((r->size >> 1) < size)
        break;
      // split
      struct run *u = (struct run*)((uint64)r + (r->size >> 1));
      r->size >>= 1;
      u->size = r->size;
      set_bitmap(u, true);
      u->next = r->next;
      u->prev = r;
      if (r->next)
        r->next->prev = u;
      r->next = u;
    }
    r = r->next;
  }
  // printf("address: %d, size: %d/%d\n", (uint64)r - HEAPSTART, size, r->size);
  return r;
}

void * 
malloc(size_t size)
{
  // 计算实际需要的大小（包括管理结构的大小）
  size_t total_size = size + sizeof(struct run);

  acquire(&hmem.lock);
  struct run *r = find_free_block(total_size);
  if (r == NULL) {
    panic("malloc: no enough heap space");
    return NULL;
  }
  delete_from_freelist(r);
  release(&hmem.lock);

  // 返回给用户的是数据区域的地址
  return (void*)((uint64)r + sizeof(struct run));
}

void
test_heap()
{
  for (size_t i = 1; i + sizeof(struct run) <= HEAP_SIZE; i <<= 1) {
    void *t = malloc(i);
    struct run *r = (struct run*)((uint64)t - sizeof(struct run));
    printf("free %d bytes at address %d(%d bytes for header)\n", r->size, (uint64)r - HEAPSTART, sizeof(struct run));
    free(t);
    show_heap();
    try_merge(hmem.freelist);
    try_merge(hmem.freelist->next);
    show_heap();
  }
  show_heap();
  struct run *head = malloc(sizeof(struct run));
  // printf("head at: %d\n", (uint64)head - HEAPSTART);
  show_heap();
  struct run *cur = head;
  for (size_t i = 1; i <= 1024; i <<= 1) {
    printf("try to allocate %d bytes\n", i + sizeof(struct run));
    void *ptr = malloc(i + sizeof(struct run));
    if (ptr == NULL) {
      printf("fail to allocate %d bytes\n", i + sizeof(struct run));
    } else {
      printf("allocated successfully: %d\n", (uint64)ptr - HEAPSTART);
    }
    cur->next = (struct run*)ptr;
    cur = cur->next;
    cur->size = i + sizeof(struct run);
    show_heap();
  }
  cur->next = NULL;
  cur = head->next;
  while (cur != NULL) {
  // for (int i=1;i<=2;i++) {
    struct run* nxt = cur->next;
    struct run *r = (struct run*)((uint64)cur - sizeof(struct run));
    printf("free %d bytes at address %d(%d bytes for header)\n", r->size, (uint64)r - HEAPSTART, sizeof(struct run));
    free(cur);
    cur = nxt;
    show_heap();
  }
  free(head);
  show_heap();
  try_merge(hmem.freelist);
  show_heap();
  printf("test done!\n");
}

