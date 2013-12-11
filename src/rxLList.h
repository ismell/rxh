#ifndef rxLList
#define rxLList
#include <stdlib.h>
#include <stdio.h>
#include "wrappers.h"

struct list_st {
	int len, *length;
	void *data;
	struct list_st *first;
	struct list_st *forward;
	struct list_st *backward;
	void (*printFunction)(void const *);
	void (*freeFunction)(void *);
};
typedef struct list_st listNode;

listNode *addItem (listNode *listpointer, void *data);
listNode *addItemp (listNode *listpointer, void *data, void (*printFunction)(void const *));
listNode *addItemf (listNode *listpointer, void *data, void (*freeFunction)(void *));
listNode *addItemfp (listNode *listpointer, void *data, void (*freeFunction)(void *), void (*printFunction)(void const *));

void printList(listNode const *lp);
void freeList(listNode *lp);

#endif
