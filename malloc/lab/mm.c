#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

team_t team = {"sunny", "sunny", "sunny@cs.cmu.edu", "", ""};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* Basic constants and marcos */
#define WSIZE 4
#define DSIZE 8

#define N 25

#define P(...) fprintf(stderr, __VA_ARGS__);fflush(stdout)

#define DE 0
#if DE == 1
#define D(...) fprintf(stderr, __VA_ARGS__);fflush(stdout)
// #define D(...)
unsigned int ALIGN(size_t size){
  return (((size) + (ALIGNMENT-1)) & ~0x7);
}
unsigned int PACK(size_t size, int alloc) {
  return ((size) | (alloc));
}
unsigned int GET(void *p) {
  return (*(unsigned int *) (p));
}
unsigned int PUT(void *p, unsigned int val) {
  return (*(unsigned int *) (p) = (val));
}
size_t GET_SIZE(void *p) {
  return (GET(p) & ~0x7);
}
size_t GET_ALLOC(void *p) {
  return (GET(p) & 0x1);
}
char *HDRP(void *bp) {
  return ((char *) (bp) - DSIZE - WSIZE);
}
char *PRED(void *bp) {
  return ((char *) (bp) - DSIZE);
}
char *SUCC(void *bp) {
  return ((char *) (bp) - WSIZE);
}
char *FTRP(void *bp) {
  return ((char *) (bp) + GET_SIZE(HDRP(bp)) - DSIZE - DSIZE);
}
char *NEXT_BLKP(void *bp) {
  return ((char *) (bp) + GET_SIZE(((char *) (bp) - DSIZE - WSIZE)));
}
char *PREV_BLKP(void *bp) {
  return ((char *) (bp) - GET_SIZE(((char *) (bp) - DSIZE - DSIZE)));
}
int MAX_SIZE(int x) {
  return 1 << x;
}
#else
#define D(...)
/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)
/* return a header / footer */
#define PACK(size, alloc) ((size) | (alloc))
/* for 4 byte pointer */
#define GET(p) (*(unsigned int*)(p))
#define PUT(p, val) (*(unsigned int*)(p) = (val))
/* for header & footer */
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)
/* for free block !  return pointer */
#define HDRP(bp) ((char*)(bp) - DSIZE - WSIZE)
#define PRED(bp) ((char*)(bp) - DSIZE)
#define SUCC(bp) ((char*)(bp) - WSIZE)
#define FTRP(bp) ((char*)(bp) + GET_SIZE(HDRP(bp)) - DSIZE - DSIZE)
/* for free block ! return pointer */
#define NEXT_BLKP(bp) ((char*)(bp) + GET_SIZE(((char*)(bp) - DSIZE - WSIZE)))
#define PREV_BLKP(bp) ((char*)(bp) - GET_SIZE(((char*)(bp) - DSIZE - DSIZE)))

#define MAX_SIZE(x) (1<<(x))
#endif

static char *heap_start;
static char *heap[N];

/* {1},{2},{3,4},{5-8},...,{1025-2048} */
size_t U(size_t size) {
  assert(size);
  size_t n = size - 1;
  n |= n >> 1;
  n |= n >> 2;
  n |= n >> 4;
  n |= n >> 8;
  n |= n >> 16;
  // n |= n >> 32; /* -m32 */
  n++;
  size_t u = 0;
  while (!(n >> u & 1)) {
    u++;
  }
  return u;
}

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void) {
  if ((heap_start = mem_sbrk(6 * WSIZE)) == (void *) -1)
    return -1;
  /* alignment padding */
  PUT(heap_start, 0);
  PUT(heap_start + (1 * WSIZE), PACK(4 * WSIZE, 1));
  PUT(heap_start + (4 * WSIZE), PACK(4 * WSIZE, 1));
  /* epilogue */
  PUT(heap_start + (5 * WSIZE), PACK(0, 1));
  for (int i = 0; i < N; ++i) {
    void *tmp;
    if ((tmp = mem_sbrk(4 * WSIZE)) == (void *) -1)
      return -1;
    tmp -= WSIZE;
    PUT(tmp, PACK(4 * WSIZE, 1)); /* header */
    PUT(tmp + WSIZE, 0); /* pred */
    PUT(tmp + DSIZE, 0); /* succ */
    PUT(tmp + (3 * WSIZE), PACK(4 * WSIZE, 1)); /* footer */
    heap[i] = tmp + DSIZE + WSIZE;
    PUT(tmp + (4 * WSIZE), PACK(0, 1));
  }
  return 0;
}

void *find_fit(size_t size) {
  for (int i = 0; i < N; ++i) {
    if (size <= MAX_SIZE(i)) {
      void *now = (void *) GET(SUCC(heap[i]));
      while (now != 0) {
        if (GET_SIZE(HDRP(now)) >= size) {
          return now;
        }
        now = (void *) GET(SUCC(now));
      }
    }
  }
  return NULL;
}
/* for debug */
void print_free() {
  for (int i = 0; i < N; i++) {
    unsigned int now = GET(SUCC(heap[i]));
    while (now != 0) {
      P("%d ", GET_SIZE(HDRP((void *) now)));
      now = GET(SUCC((void *) now));
    }
  }
  P("\n-----------\n");
}

void heap_remove(void *bp) {
  void *pred = (void *) GET(PRED(bp));
  void *succ = (void *) GET(SUCC(bp));
  assert(pred != 0);
  PUT(SUCC(pred), (unsigned int) succ);
  if (succ != 0) {
    PUT(PRED(succ), (unsigned int) pred);
  }
  /* clear header & footer */
  PUT(PRED(bp), 0);
  PUT(SUCC(bp), 0);
}

void heap_add(void *bp) {
  void *head = heap[U(GET_SIZE(HDRP(bp)))];
  void *nxt = (void *) GET(SUCC(head));

  PUT(SUCC(bp), (unsigned int) nxt);
  if (nxt != 0) {
    PUT(PRED(nxt), (unsigned int) bp);
  }

  PUT(SUCC(head), (unsigned int) bp);
  PUT(PRED(bp), (unsigned int) head);
}

/* will maintain the heap list */
static void *coalesce(void *bp) {
  void *pre = PREV_BLKP(bp);
  void *nxt = NEXT_BLKP(bp);
  size_t prev_alloc = GET_ALLOC(FTRP(pre));
  size_t next_alloc = GET_ALLOC(HDRP(nxt));
  size_t size = GET_SIZE(HDRP(bp));
  /* bp won't in heap list */
  //heap_remove(bp);

  if (!prev_alloc) {
    size += GET_SIZE(HDRP(pre));
    heap_remove(pre);
    bp = PREV_BLKP(bp);
  }
  if (!next_alloc) {
    size += GET_SIZE(HDRP(nxt));
    heap_remove(nxt);
  }

  PUT(HDRP(bp), PACK(size, 0));
  PUT(FTRP(bp), PACK(size, 0));
  heap_add(bp);
  return bp;
}

static void *extend_heap(size_t words) {
  char *bp;
  size_t size;

  /* Allocate an even number of words to maintain alignment */
  size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
  if ((long) (bp = mem_sbrk(size)) == -1)
    return NULL;
  // bp += DSIZE + WSIZE; /* fix bug */
  bp += DSIZE;
  /* free header & pred & succ & footer */
  PUT(HDRP(bp), PACK(size, 0));
  PUT(FTRP(bp), PACK(size, 0));
  /* new epilogue */
  PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));

  /* coalesce if the previous block was free */
  return coalesce(bp);
}

static void *place(void *bp, size_t asize) {

  size_t size = GET_SIZE(HDRP(bp));

  /* remove from free list */
  heap_remove(bp);
  size_t rest = size - asize;
  size_t alloc_size = (rest >= 4 * WSIZE) ? asize : size;
  /* set the allocated block's header & footer */
  PUT(HDRP(bp), PACK(alloc_size, 1));
  PUT(FTRP(bp), PACK(alloc_size, 1));
  if (rest >= 4 * WSIZE) {
    /* p might be footer, but it's ok ? */
    void *p = NEXT_BLKP(bp);
    /* set the rest's header & footer */
    PUT(HDRP(p), PACK(rest, 0));
    PUT(FTRP(p), PACK(rest, 0));
    coalesce(p);
  }
  /* discard pred & succ when allocated */
  return (bp - DSIZE);
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size) {
  if (size == 0) {
    return NULL;
  }
  size_t asize;
  size_t extendsize;
  char *bp;
  // overflow ?
  if (size <= DSIZE)
    asize = 2 * DSIZE;
  else
    asize = DSIZE * ((size + DSIZE + (DSIZE - 1)) / DSIZE);

  if ((bp = find_fit(asize)) != NULL) {
    return place(bp, asize);
  }

  // extendsize = MAX(asize, CHUNKSIZE);
  extendsize = asize;
  if ((bp = extend_heap(extendsize / WSIZE)) == NULL)
    return NULL;
  return place(bp, asize);
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr) {
  ptr += DSIZE;
  size_t size = GET_SIZE(HDRP(ptr));

  PUT(HDRP(ptr), PACK(size, 0));
  PUT(FTRP(ptr), PACK(size, 0));

  coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size) {
  if (ptr == NULL) {
    return mm_malloc(size);
  }
  if (size == 0) {
    mm_free(ptr);
    return NULL;
  }
  size_t old_size = GET_SIZE(HDRP(ptr + DSIZE)) - DSIZE;
  if (size <= old_size) {
    return ptr;
  } else {
    void *nxt_block = NEXT_BLKP(ptr + DSIZE);
    void *nxt_header = HDRP(nxt_block);
    /* it's the last block */
    if (GET_SIZE(nxt_header) == 0) {
      size_t d = size - old_size;
      /* should not use mm_malloc */
      if (mem_sbrk(ALIGN(d)) == (void *) -1) {
        return NULL;
      }
      unsigned int new_size = GET_SIZE(HDRP(ptr + DSIZE)) + ALIGN(d);
      PUT(HDRP(ptr + DSIZE), PACK(new_size, 1));
      PUT(FTRP(ptr + DSIZE), PACK(new_size, 1));
      /* new epilogue */
      PUT(FTRP(ptr + DSIZE) + WSIZE, PACK(0, 1));
      return ptr;
    } else if (GET_ALLOC(nxt_header) == 0 && (GET_SIZE(nxt_header) + old_size >= size)) {
      /* use the next free block */
      heap_remove(nxt_block);
      unsigned int new_size = GET_SIZE(HDRP(ptr + DSIZE)) + GET_SIZE(HDRP(nxt_block));
      PUT(HDRP(ptr + DSIZE), PACK(new_size, 1));
      PUT(FTRP(ptr + DSIZE), PACK(new_size, 1));
      return ptr;
    }
  }
  void *newptr;
  if ((newptr = mm_malloc(size)) == NULL) {
    return NULL;
  }
  memcpy(newptr, ptr, old_size);
  mm_free(ptr);
  return newptr;
}