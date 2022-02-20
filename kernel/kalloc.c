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

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

struct {

  // lock for reference count array
  struct spinlock lock;

  // reference count array
  uint ref_count[PHYSTOP/PGSIZE];
} reference_count;

// gets the reference count index for the given physical address
uint64 get_ref_count_index(void *pa) { return ((uint64)pa) / PGSIZE; }

// gets the reference count lock
void acquire_ref_count_lock(void) {
  acquire(&reference_count.lock);
  return;
}

// releases the reference count lock
void release_ref_count_lock(void) {
  release(&reference_count.lock);
  return;
}

// gets the reference count for the given physical address
uint get_ref_count(void *pa) {

  // get the index of the reference count array
  uint64 index = get_ref_count_index(pa);

  // get the reference count
  uint ref_count = reference_count.ref_count[index];

  // return the reference count
  return ref_count;
}

// sets the reference count for the given physical address to the given value
void set_ref_count(void *pa, uint val) {

  // get the index of the reference count array
  uint64 index = get_ref_count_index(pa);

  // get the reference count
  reference_count.ref_count[index] = val;

  return;
}

void
kinit()
{
  initlock(&kmem.lock, "kmem");

  // initialize the reference count array lock
  initlock(&reference_count.lock, "reference_count");

  // set all reference counts to zero
  memset(reference_count.ref_count, 0, sizeof(reference_count.ref_count));

  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE) {

    // get the reference counter lock
    acquire(&reference_count.lock);

    // set the reference count to 1 in order to initialize it
    set_ref_count((void *)p, 1);

    // release the reference counter lock
    release(&reference_count.lock);

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

  // get the reference counter lock
  acquire(&reference_count.lock);

  // get the reference count
  uint ref_count = get_ref_count(pa);

  // if the reference count is 1, then page should not be freed
  if (ref_count > 1) {
    set_ref_count(pa, ref_count - 1);
    release(&reference_count.lock);
    return;
  }

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // since we've reached this point of the function
  // set the reference count to 0, page should be freed
  set_ref_count(pa, 0);

  // release the reference counter lock
  release(&reference_count.lock);

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
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk

  // set the newly allocated page's reference count to 1
  // since it is the first reference to the page
  if (r) {
    // get reference count lock
    acquire(&reference_count.lock);

    // set the reference count to 1
    set_ref_count((void *)r, 1);

    // release the reference count lock
    release(&reference_count.lock);
  }

  return (void*)r;
}
