#include "pool.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct node {
  int start;
  int end;
  struct node *next;
};

struct pool {
  struct node *root;
  int maxsize;
  char *array;
};

struct pool *pool_create(int size) {
  assert(size > 0);
  struct pool *new = malloc(sizeof(struct pool));
  new->root = NULL;
  new->maxsize = size;
  new->array = malloc(size * sizeof(char));
  return new;
}

bool pool_destroy(struct pool *p) {
  assert(p);
  if (p->root) {
    return false;
  }
  free(p->root);
  free(p->array);
  free(p);
  return true;
}

static struct node *new_node(int start, int size) {
  struct node *new = malloc(sizeof(struct node));
  new->start = start;
  new->end = start + size;
  new->next = NULL;
  return new;
} 

char *pool_alloc(struct pool *p, int size) {
  assert(p);
  assert(size > 0);
  if (p->root == NULL) { // if no memory has been allocated
    if (p->maxsize >= size) { // and pool has enough size
      struct node *root = new_node(0, size);
      p->root = root;
      return &p->array[0];
    } else { // pool does not have enough size
      return NULL;
    }
  }
  // if pool already has other nodes
  int start = 0;
  int end = 0;
  int space = 0;
  struct node *curnode = p->root;
  // first check the space before the first node
  space = curnode->start;
  if (space >= size) {
    struct node *new_root = new_node(0, size);
    p->root = new_root;
    new_root->next = curnode;
    return &p->array[0];
  }
  start = curnode->end;
  // then, check all space in between nodes
  while (curnode->next) {
    end = curnode->next->start;
    space = end - start;
    if (space >= size) { // if there's enough space to allocate
      struct node *new = new_node(start, size);
      new->next = curnode->next;
      curnode->next = new;
      return &p->array[start];
    }
    curnode = curnode->next;
    start = curnode->end;
  }
  // lastly, check if there's enough space after the last node
  space = p->maxsize - start;
  if (space >= size) { 
    struct node *new = new_node(start, size);
    curnode->next = new;
    return &p->array[start];
  }
  return NULL;
}

bool pool_free(struct pool *p, char *addr) {
  assert(p);
  if ((! addr) || (! p->root)) { // addr is NULL or no memory has been allocated
    return false;
  }
  // check if root is the target node
  struct node *curnode = p->root;
  if (&p->array[curnode->start] == addr) {
    struct node *new_root = curnode->next;
    free(curnode);
    p->root = new_root;
    return true;
  }
  // check the rest nodes in the pool
  while (curnode->next) {
    if (&p->array[curnode->next->start] == addr) {
      struct node *next = curnode->next->next;
      free(curnode->next);
      curnode->next = next;
      return true;
    }
    curnode = curnode->next;
  }
  return false;
}


char *pool_realloc(struct pool *p, char *addr, int size) {
  assert(p);
  assert(size > 0);
  struct node *curnode = p->root;
  while (curnode) {
    if (&p->array[curnode->start] == addr) {
      int old_size = curnode->end - curnode->start;

      // 3 scenarios where we return the original address:
      if ((size <= old_size) || 
          // if new size <= allocated size
          (curnode->next && 
           (size <= (curnode->next->start - curnode->start))) ||
          // or if enough space between two nodes
          ((! curnode->next) && 
           (size <= p->maxsize - curnode->start))) {
        // or if curnode is the last node and there's enough space after it
        curnode->end = curnode->start + size;
        return &p->array[curnode->start];

      } else { // if there ain't enough space in the orginal addr
        // try allocating new space
        char *new_addr = pool_alloc(p, size);
        if (new_addr) { // if new address is valid
          pool_free(p, addr); // free the old node and return new
          return memcpy(new_addr, addr, old_size);
        }
        return NULL; // if allocation fails
      }    
    }
    curnode = curnode->next;
  }
  return NULL; // if addr doesn't exist in p or if root is NULL
}

void pool_print_active(const struct pool *p) {
  assert(p);
  if (! p->root) { // if no active allocations
    printf("active: none\n");
    return;
  }
  printf("active:");
  struct node *curnode = p->root;
  while (curnode) {
    printf(" %d ", curnode->start);
    printf("[%d]", curnode->end - curnode->start);
    if (curnode->next) {
      printf(",");
    }
    curnode = curnode->next;
  }
  printf("\n");
}

void pool_print_available(const struct pool *p) {
  assert(p);
  // a flag to tell if there's any memory available:
  int NON_AVAILABLE = 1;
  // a flag that indicates if there has previously been available memory:
  int FIRST_SPACE = 1; 
  printf("available:");
  struct node *curnode = p->root;
  int start = 0;
  int end = 0;
  while (curnode) {
    end = curnode->start;
    int space = end - start; // avaiable space
    if (space > 0) {
      if (FIRST_SPACE == 0) { // print "," if isn't the first available memory
        printf(",");
      }
      FIRST_SPACE = 0;
      NON_AVAILABLE = 0;
      printf(" %d ", start);
      printf("[%d]", end - start);
    }
    start = curnode->end;
    curnode = curnode->next;
  }
  // lastly, check for available memory after the last node:
  if (p->maxsize - start > 0) {
    NON_AVAILABLE = 0;
    printf(" %d ", start);
    printf("[%d]", p->maxsize - start);
  }
  if (NON_AVAILABLE == 1) { // if there is no memory available
    printf(" none");
  }
  printf("\n");
}
