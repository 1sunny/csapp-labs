#include "lru.h"
#include <stdlib.h>
#include <string.h>

#define READER_SAME_TIME 10

LRUCache *lru_init(int capacity) {
  LRUCache *lru = (LRUCache *) malloc(sizeof(LRUCache));
  lru->capacity = capacity;
  lru->head = (linkList *) malloc(sizeof(linkList));
  lru->tail = (linkList *) malloc(sizeof(linkList));
  lru->head->next = lru->tail;
  lru->head->pre = NULL;
  lru->tail->pre = lru->head;
  lru->tail->next = NULL;
  // read-write lock init
  lru->readcnt = 0;
  sem_init(&lru->w, 0, 1);
  sem_init(&lru->mutex, 0, READER_SAME_TIME);
  return lru;
}

void lru_remove(linkList *list) {
  list->next->pre = list->pre;
  list->pre->next = list->next;
}

void push_back(LRUCache *lru, linkList *list) {
  lru->tail->pre->next = list;
  list->pre = lru->tail->pre;
  list->next = lru->tail;
  lru->tail->pre = list;
}

linkList *lru_find(LRUCache *lru, char *key) {
  linkList *curr = lru->head->next;
  while (curr != lru->tail) {
    if (strcmp(curr->key, key) == 0) {
      return curr;
    }
    curr = curr->next;
  }
  return NULL;
}

void dec_reader(LRUCache *lru) {
  sem_wait(&lru->mutex);
  lru->readcnt--;
  if (lru->readcnt == 0) {
    sem_post(&lru->w);
  }
  sem_post(&lru->mutex);
}

cacheData *lru_get(LRUCache *lru, char *key) {
  // P
  sem_wait(&lru->mutex);
  lru->readcnt++;
  if (lru->readcnt == 1) {
    sem_wait(&lru->w);
  }
  sem_post(&lru->mutex);

  linkList *tar = lru_find(lru, key);
  if (tar) {
    lru_remove(tar);
    push_back(lru, tar);
    // V
    dec_reader(lru);
    return tar->data;
  }
  // V
  dec_reader(lru);
  return NULL;
}

void lru_put(LRUCache *lru, char *key, cacheData *value) {
  sem_wait(&lru->w);

  linkList *tar = lru_find(lru, key);
  if (tar) {
    lru_remove(tar);
    tar->data = value;
    push_back(lru, tar);
  } else {
    if (lru->capacity == lru->size) {
      lru_remove(lru->head->next);
    }
    linkList *list = (linkList *) malloc(sizeof(linkList));
    strcpy(list->key, key);
    list->data = value;
    push_back(lru, list);
    lru->size++;
  }

  sem_post(&lru->w);
}