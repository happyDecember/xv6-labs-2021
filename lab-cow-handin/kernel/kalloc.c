// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

int page_index_count[PHYSTOP / PGSIZE];  // add
struct spinlock ref_count_lock;

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE){	  
    kfree(p);
  }  
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{	  

  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");
  
  acquire(&ref_count_lock);
  page_index_count[(uint64)pa / PGSIZE]-=1;
  int temp = page_index_count[(uint64)pa / PGSIZE];	  
  release(&ref_count_lock);
  if(temp > 0){
    return;
  }
   // add

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r){
    kmem.freelist = r->next;
    acquire(&ref_count_lock);
    page_index_count[(uint64)r / PGSIZE] = 1;  // add
    release(&ref_count_lock);
  }  
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}

/*void
index_plusplus(uint64 pa)
{
  acquire(&(kmem.lock));
  page_index_count[pa / PGSIZE]++;
  release(&(kmem.lock));
}

void
index_subsub(uint64 pa)
{
  acquire(&(kmem.lock));
  page_index_count[pa / PGSIZE]--;
  release(&(kmem.lock));
}*/  

int
cowfault(pagetable_t pagetable, uint64 va)
{	
  char *mem;
  if(va >= MAXVA)
    return -1; 
  
  pte_t* pte = walk(pagetable, va, 0);  // 获取地址对应页表项
  
  if(!pte || !(*pte & PTE_COW)|| !(*pte & PTE_U) || !(*pte & PTE_V))
    return -1;  
  if((mem = kalloc()) == 0)
    return -1;	  
  memmove((char*)mem, (char*)PTE2PA(*pte), PGSIZE);
  kfree((void*)PTE2PA(*pte));
  uint flags = PTE_FLAGS(*pte);
  *pte = (PA2PTE((uint64)mem) | flags | PTE_W);
  *pte &= (~PTE_COW);    
  return 0;
}



