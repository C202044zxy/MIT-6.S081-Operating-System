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

#define NTABLE 13
extern struct spinlock tickslock;
extern uint ticks;

struct {
  struct spinlock lock[NTABLE];
  struct buf buf[NBUF];
  struct buf* head[NTABLE];
} bcache;

void
binit(void)
{
  struct buf *b;

  for (int i = 0; i < NTABLE; i++) {
    initlock(&(bcache.lock[i]), "bcache");
    bcache.head[i] = 0;
  }
  // put all bufs into 0 bucket
  for (b = bcache.buf; b < bcache.buf + NBUF; b++) {
    b->next = bcache.head[0];
    if (bcache.head[0]) {
      bcache.head[0]->prev = b;
    }
    bcache.head[0] = b;
    initsleeplock(&b->lock, "buffer");
    b->time = 0;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  int id = blockno % NTABLE;
  acquire(&(bcache.lock[id]));

  // Is the block already cached?
  for(b = bcache.head[id]; b; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&(bcache.lock[id]));
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  // borrow from other table
  struct buf *victim;
  int tid = -1;
  uint time = -1;
  // printf("%d\n", id);
  for (int i = 1; i < NTABLE; i++) {
    int x = (id + i) % NTABLE;
    // printf("find %d\n", x);
    acquire(&(bcache.lock[x]));
    for (b = bcache.head[x]; b; b = b->next) {
      if (b->refcnt == 0) {
        // can be evicted
        if (time == -1 || b->time < time) {
          // choose the smallest time stamp
          time = b->time;
          // release the table lock of previous choice
          if (tid != -1 && tid != x) {
            release(&(bcache.lock[tid]));
          }
          tid = x;
          victim = b;
        }
      }
    }
    if (tid == -1 || tid != x) {
      release(&(bcache.lock[x]));
    }
  }
  if (tid != -1) {
    // now we hold the lock of tid and id
    victim->dev = dev;
    victim->blockno = blockno;
    victim->valid = 0;
    victim->refcnt = 1;
    // remove the victim from tid
    if (victim->prev) {
      victim->prev->next = victim->next;
    }
    if (victim->next) {
      victim->next->prev = victim->prev;
    }
    if (victim == bcache.head[tid]) {
      bcache.head[tid] = victim->next;
    }
    // add the victim to id
    victim->next = bcache.head[id];
    if (victim->next) {
      victim->next->prev = victim;
    }
    victim->prev = 0;
    bcache.head[id] = victim;
    // release all the locks 
    release(&(bcache.lock[id]));
    release(&(bcache.lock[tid]));
    acquiresleep(&victim->lock);
    return victim;
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

  int id = b->blockno % NTABLE;
  releasesleep(&b->lock);

  acquire(&(bcache.lock[id]));
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    acquire(&tickslock);
    b->time = ticks;
    release(&tickslock);
  }
  
  release(&(bcache.lock[id]));
}

void
bpin(struct buf *b) {
  int id = b->blockno % NTABLE;
  acquire(&(bcache.lock[id]));
  b->refcnt++;
  release(&(bcache.lock[id]));
}

void
bunpin(struct buf *b) {
  int id = b->blockno % NTABLE;
  acquire(&(bcache.lock[id]));
  b->refcnt--;
  release(&(bcache.lock[id]));
}