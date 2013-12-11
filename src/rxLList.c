#include "rxLList.h"

listNode *addItem (listNode *listpointer, void *data) {
	return addItemfp(listpointer, data, NULL, NULL);
}
listNode *addItemp (listNode *listpointer, void *data, void (*printFunction)(void const *)) {
	return addItemfp(listpointer, data, NULL, printFunction);
}
listNode *addItemf (listNode *listpointer, void *data, void (*freeFunction)(void *)) {
	return addItemfp(listpointer, data, freeFunction, NULL);
}
listNode * addItemfp (listNode *listpointer, void *data, void (*freeFunction)(void *), void (*printFunction)(void const *)) {
	listNode *lp = xmalloc (sizeof (listNode)); // Allocate Memory

	if (listpointer != NULL) {
		lp->first = listpointer->first;  // Set the First Node
		lp->length = lp->first->length;  // Set the Length Pointer
		(*lp->length)++;                 // Increment the list length
		lp->data = data;                 // Set the Data
		lp->printFunction = (printFunction != NULL ? printFunction : lp->first->printFunction);
		if (listpointer->forward == NULL) {
			listpointer->forward = lp;
			lp->backward = listpointer;
			lp->forward = NULL;
		} else {
			lp->forward = listpointer->forward;
			listpointer->forward = lp;
			lp->backward = listpointer;
			lp->forward->backward = lp;
		}
		return lp;
	} else {
		lp->forward = NULL;             // No Forward Node
		lp->backward = NULL;            // No Backward Node
		lp->len = 1;
		lp->length = &lp->len;          // Reference the length pointer to len
		lp->first = lp;                 // Set its self as the first node
		lp->data = data;                // Set the data
		lp->printFunction = printFunction; // Set the printFunction
		return lp;
	}
}

void printList(listNode const *lp) {
	while (lp != NULL) {
		if (lp->printFunction != NULL)
			lp->printFunction(lp->data);
		else if (lp->first != NULL && lp->first->printFunction != NULL)
			lp->first->printFunction(lp->data);
		else
			printf("Skipping Node\n");
		lp = lp->forward;
	}
}

void freeList(listNode *lp) {
	
}
