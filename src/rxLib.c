#include "rxLib.h"

char *charpos(char const *haystack, char const *needles, int offset) {
	if (haystack == NULL || needles == NULL)
		return NULL;
	int i, j;
	for (i = 0 + offset; needles[i] != 0; i++) {
		for (j = 0; needles[j] != 0; j++) {
			if (haystack[i] == needles[j]) {
				return (char*)haystack+i;
			}
		}
	}
	return NULL;
}

char *charrpos(char const *haystack, char const *needles, int offset) {
	if (haystack == NULL || needles == NULL)
		return NULL;
	int i, j;
	for (i = strlen(haystack) - 1 - offset; i >= 0; i--) {
		for (j = 0; needles[j] != 0; j++) {
			if (haystack[i] == needles[j]) {
				return (char*)haystack+i;
			}
		}
		if (haystack[i] != ' ')
			break;
	}
	return NULL;
}

bool quickSorti(int *arr, int elements) {
	#define MAX_LEVELS 1000
	int  piv, beg[MAX_LEVELS], end[MAX_LEVELS], i=0, L, R ;
	beg[0]=0; end[0]=elements;

	while (i>=0) {
		L=beg[i]; R=end[i]-1;
		if (L<R) {
			piv=arr[L];
			if (i==MAX_LEVELS-1)
				return false;
			while (L<R) {
				while (arr[R]>=piv && L<R) R--;
				if (L<R) arr[L++]=arr[R];
				while (arr[L]<=piv && L<R) L++;
				if (L<R) arr[R--]=arr[L];
			}
			arr[L]=piv;
			beg[i+1]=L+1;
			end[i+1]=end[i];
			end[i++]=L;
		}
		else {
			i--;
		}
	}
	return true;
}

bool quickSortc(char **arr, int elements) {
	#define MAX_LEVELS 1000
	int  beg[MAX_LEVELS], end[MAX_LEVELS], i=0, L, R;
	char *piv;
	beg[0]=0; end[0]=elements;

	while (i>=0) {
		L=beg[i]; R=end[i]-1;
		if (L<R) {
			piv=arr[L];
			if (i==MAX_LEVELS-1)
				return false;
			while (L<R) {
				while (strcmp(arr[R],piv) >= 0 && L<R) R--;
				if (L<R) arr[L++]=arr[R];
				while (strcmp(arr[L],piv) <= 0 && L<R) L++;
				if (L<R) arr[R--]=arr[L];
			}
			arr[L]=piv;
			beg[i+1]=L+1;
			end[i+1]=end[i];
			end[i++]=L;
		}
		else {
			i--;
		}
	}
	return true;
}
