// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define NBUCKETS 31  // add

struct {
  struct spinlock lock[NBUCKETS]; //change
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head[NBUCKETS]; //change
} bcache;

void
binit(void)
{
  struct buf *b;

  for(int i = 0; i < NBUCKETS; i++){
    initlock(&bcache.lock[i], "bcache"); //change

  // Create linked list of buffers
    bcache.head[i].prev = &bcache.head[i]; //change
    bcache.head[i].next = &bcache.head[i]; //change
  }

  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->next = bcache.head[0].next;
    b->prev = &bcache.head[0];
    initsleeplock(&b->lock, "buffer");
    bcache.head[0].next->prev = b;
    bcache.head[0].next = b;
  } //change
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  int hashvalue = blockno % NBUCKETS;  // add

  //acquire(&bcache.lock);
  acquire(&bcache.lock[hashvalue]);  // change

  // Is the block already cached?
  for(b = bcache.head[hashvalue].next; b != &bcache.head[hashvalue]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.lock[hashvalue]);
      acquiresleep(&b->lock);
      return b;
    } //change
  }

  for(int i = (hashvalue + 1) % NBUCKETS; i % NBUCKETS != hashvalue; i = (i + 1) % NBUCKETS){  // add
    acquire(&(bcache.lock[i]));  // add	  
  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
    for(b = bcache.head[i].prev; b != &bcache.head[i]; b = b->prev){  //change
      if(b->refcnt == 0) {
        b->dev = dev;
        b->blockno = blockno;
        b->valid = 0;
        b->refcnt = 1;

	//add
	b->next->prev = b->prev;
	b->prev->next = b->next;
	b->next = bcache.head[hashvalue].next;
	b->prev = &bcache.head[hashvalue]; 
	bcache.head[hashvalue].next->prev = b;
	bcache.head[hashvalue].next = b;
	release(&bcache.lock[i]); //add

        release(&bcache.lock[hashvalue]); //change
        acquiresleep(&b->lock);
        return b;
      }
    }
    release(&(bcache.lock[i]));  // add
  }
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);
  int hashvalue = b->blockno % NBUCKETS;  // add

  //acquire(&bcache.lock);
  acquire(&bcache.lock[hashvalue]);  // change

  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    //b->next = bcache.head.next;
    b->next = bcache.head[hashvalue].next;  // change
    //b->prev = &bcache.head;
    b->prev = &bcache.head[hashvalue];  // change
    //bcache.head.next->prev = b;
    bcache.head[hashvalue].next->prev = b;  // change
    //bcache.head.next = b;
    bcache.head[hashvalue].next = b;  // change
  }
  
  //release(&bcache.lock);
  release(&bcache.lock[hashvalue]);  // change
}

void
bpin(struct buf *b) {
  int hashvalue = b->blockno % NBUCKETS;  // add	
  //acquire(&bcache.lock);
  acquire(&bcache.lock[hashvalue]);  // change
  b->refcnt++;
  //release(&bcache.lock);
  release(&bcache.lock[hashvalue]);  // change
}

void
bunpin(struct buf *b) {
  int hashvalue = b->blockno % NBUCKETS;  // add	
  //acquire(&bcache.lock);
  acquire(&bcache.lock[hashvalue]);  // change
  b->refcnt--;
  //release(&bcache.lock);
  release(&bcache.lock[hashvalue]);  // change
}


