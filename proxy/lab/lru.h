#ifndef __LRU_H__
#define __LRU_H__

#include <semaphore.h>

#define  MAXLINE   8192  /* Max text line length */
#define MAX_OBJECT_SIZE 102400

typedef struct cacheData {
    char buf[MAX_OBJECT_SIZE];
    int len;
} cacheData;

typedef struct linkList {
    struct linkList *pre, *next;
    char key[MAXLINE];
    cacheData *data;
} linkList;

typedef struct LRUCache {
    int capacity;
    int size;
    linkList *head, *tail;
    int readcnt;
    sem_t mutex, w; /* Both initially = 1 */
} LRUCache;

LRUCache *lru_init(int capacity);

cacheData *lru_get(LRUCache *lru, char *key);

void lru_put(LRUCache *lru, char *key, cacheData *value);

#endif
