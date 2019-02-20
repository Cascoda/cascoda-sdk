
/**
	@file cascoda_linked_list.c
	@brief Implementation of a doubly-linked list
	@author Andrew Howe
*//*
* Copyright (C) 2016  Cascoda, Ltd.
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cascoda-bm/cascoda_linked_list.h"

#define LLIST_CHECK_NULLP (true)

static inline bool nullpCheck(const void *ptr)
{
	if (ptr == NULLP)
	{
		printf("WARNING: IN CASCODA_LINKED_LIST NULLP GIVEN\n");
		return true;
	}
	else
	{
		return false;
	}
}

LLIST_Node_t *LLIST_new(u8_t size)
{
	LLIST_Node_t *llnew = (LLIST_Node_t *)malloc(sizeof(LLIST_Node_t));
	if (llnew == NULLP)
	{
		return NULLP;
	}
	memset(llnew, 0, sizeof(LLIST_Node_t));
	llnew->value = malloc(size);
	if (llnew->value == NULLP)
	{
		free((u8_t *)llnew);
		return NULLP;
	}
	memset(llnew->value, 0, size);
	return llnew;
}

LLIST_Node_t *LLIST_delete(LLIST_Node_t *node)
{
	if (LLIST_CHECK_NULLP && nullpCheck(node))
	{
		return NULLP;
	}
	LLIST_Node_t *next = node->next;
	LLIST_moveAfter(node, NULLP);
	free((u8_t *)node->value);
	free((u8_t *)node);
	return next;
}

LLIST_Node_t *LLIST_getLast(const LLIST_Node_t *node)
{
	if (LLIST_CHECK_NULLP && nullpCheck(node))
	{
		return NULLP;
	}
	while (node->next != NULLP)
	{
		node = node->next;
	}
	return (LLIST_Node_t *)node;
}

LLIST_Node_t *LLIST_getFirst(const LLIST_Node_t *node)
{
	if (LLIST_CHECK_NULLP && nullpCheck(node))
	{
		return NULLP;
	}
	while (node->prev != NULLP)
	{
		node = node->prev;
	}
	return (LLIST_Node_t *)node;
}

void LLIST_moveAfter(LLIST_Node_t *node, LLIST_Node_t *newParent)
{
	if (LLIST_CHECK_NULLP && nullpCheck(node))
	{
		return;
	}
	LLIST_Node_t *oldNodeNext = node->next;
	LLIST_Node_t *oldNodePrev = node->prev;
	if (oldNodeNext != NULLP)
		oldNodeNext->prev = oldNodePrev;
	if (oldNodePrev != NULLP)
		oldNodePrev->next = oldNodeNext;

	LLIST_Node_t *newNodeNext = newParent == NULLP ? NULLP : newParent->next;
	LLIST_Node_t *newNodePrev = newParent;
	node->next                = newNodeNext;
	node->prev                = newNodePrev;

	if (newNodeNext != NULLP)
		newNodeNext->prev = node;
	if (newNodePrev != NULLP)
		newNodePrev->next = node;
}

void LLIST_moveBefore(LLIST_Node_t *node, LLIST_Node_t *newChild)
{
	if (LLIST_CHECK_NULLP && nullpCheck(node))
	{
		return;
	}
	LLIST_Node_t *oldNodeNext = node->next;
	LLIST_Node_t *oldNodePrev = node->prev;
	if (oldNodeNext != NULLP)
		oldNodeNext->prev = oldNodePrev;
	if (oldNodePrev != NULLP)
		oldNodePrev->next = oldNodeNext;

	LLIST_Node_t *newNodeNext = newChild;
	LLIST_Node_t *newNodePrev = newChild == NULLP ? NULLP : newChild->prev;
	node->next                = newNodeNext;
	node->prev                = newNodePrev;

	if (newNodeNext != NULLP)
		newNodeNext->prev = node;
	if (newNodePrev != NULLP)
		newNodePrev->next = node;
}

void LLIST_moveToEnd(LLIST_Node_t *node, LLIST_Node_t *list)
{
	LLIST_Node_t *last = NULLP;
	if (list != NULLP)
	{
		last = LLIST_getLast(list);
	}
	if (node != last)
	{
		LLIST_moveAfter(node, last);
	}
}

void LLIST_moveToStart(LLIST_Node_t *node, LLIST_Node_t *list)
{
	LLIST_Node_t *first = NULLP;
	if (list != NULLP)
	{
		first = LLIST_getFirst(list);
	}
	if (node != first)
	{
		LLIST_moveBefore(node, first);
	}
}

void LLIST_moveSort(LLIST_Node_t *node, LLIST_Node_t *list, LLIST_Compare_func *compare)
{
	if (LLIST_CHECK_NULLP && nullpCheck(node))
	{
		return;
	}

	if (list == NULLP)
	{
		LLIST_moveAfter(node, list);
		return;
	}

	// go to last list node that is not higher than the new deadline
	while (list->next != NULLP && compare(list->next, node) <= 0)
	{
		list = list->next;
	}

	if (compare(list, node) > 0)
	{
		LLIST_moveBefore(node, list);
	}
	else
	{ //list <= node
		LLIST_moveAfter(node, list);
	}
}

void LLIST_join(LLIST_Node_t *parent, LLIST_Node_t *child)
{
	if (child != NULLP)
	{
		if (child->prev != NULLP)
			child->prev->next = NULLP;
		child->prev = parent;
	}
	if (parent != NULLP)
	{
		if (parent->next != NULLP)
			parent->next->prev = NULLP;
		parent->next = child;
	}
}

LLIST_Node_t *LLIST_sort(LLIST_Node_t *list, LLIST_Compare_func *compare)
{
	LLIST_Node_t *p;
	LLIST_Node_t *l         = list;
	u8_t          k         = 1;
	u8_t          numMerges = 0;

	do
	{
		p         = LLIST_getFirst(l);
		l         = NULLP;
		numMerges = 0;
		while (p != NULLP)
		{
			numMerges++;
			LLIST_Node_t *q = p;
			u8_t          psize;
			for (psize = 0; psize < k; psize++)
			{
				if (q->next == NULLP)
				{
					break;
				}
				else
				{
					q = q->next;
				}
			}
			u8_t qsize = k;

			bool chooseP;
			while (psize > 0 || (qsize > 0 && q != NULLP))
			{
				if (psize == 0 || p == NULLP)
				{
					chooseP = false;
				}
				else if (qsize == 0 || q == NULLP)
				{
					chooseP = true;
				}
				else
				{
					i8_t result = compare(p, q);
					if (result <= 0)
					{ //p <= q
						chooseP = true;
					}
					else
					{ //result > 0,  p > q
						chooseP = false;
					}
				}
				if (chooseP)
				{
					LLIST_Node_t *nextp = p->next;
					LLIST_moveAfter(p, l);
					l = p;
					p = nextp;
					psize--;
				}
				else
				{
					LLIST_Node_t *nextq = q->next;
					LLIST_moveAfter(q, l);
					l = q;
					q = nextq;
					qsize--;
				}
			}
			p = q;
		}
		k *= 2;
	} while (numMerges > 1);
	return LLIST_getLast(l);
}

LLIST_Node_t *LLIST_find(LLIST_Node_t *list, LLIST_Iter_func *check)
{
	while (list != NULLP)
	{
		LLIST_Node_t *nextList = list->next;
		i8_t          result   = check(list);
		if (result > 0)
		{
			break;
		}
		else if (result == 0)
		{
			list = nextList;
		}
		else
		{ //negative
			return NULLP;
		}
	}
	return list;
}

u8_t LLIST_foreach(LLIST_Node_t *list, LLIST_Iter_func *func)
{
	u8_t count = 0;
	while (list != NULLP)
	{
		LLIST_Node_t *nextList = list->next;
		i8_t          result   = func(list);
		if (result > 0)
		{
			count++;
		}
		else if (result < 0)
		{
			break;
		}
		list = nextList;
	}
	return count;
}

u8_t LLIST_destroy(LLIST_Node_t *list)
{
	list                = LLIST_getFirst(list);
	u8_t          count = 0;
	LLIST_Node_t *next  = NULLP;
	while (list != NULLP)
	{
		next = list->next;
		free((u8_t *)list->value);
		free((u8_t *)list);
		list = next;
		count++;
	}
	return count;
}

static LLIST_Compare_func unitTestCompare;
static LLIST_Iter_func    unitTestFind;
static LLIST_Iter_func    unitTestIter;

bool LLIST_unitTest(void)
{
	LLIST_Node_t *A;
	LLIST_Node_t *B;
	LLIST_Node_t *C;
	A = LLIST_new(1);
	B = LLIST_new(1);
	C = LLIST_new(1);
	if (A == NULLP)
		return false;
	if (B == NULLP)
		return false;
	if (C == NULLP)
		return false;
	*((u8_t *)A->value) = 1;
	*((u8_t *)B->value) = 2;
	*((u8_t *)C->value) = 3;

	u8_t *Aval = (u8_t *)A->value;
	u8_t *Bval = (u8_t *)B->value;
	u8_t *Cval = (u8_t *)C->value;
	if (*Aval != 1)
		return false;
	if (*Bval != 2)
		return false;
	if (*Cval != 3)
		return false;

	LLIST_moveAfter(A, B);
	//B->A
	if (A->prev != B)
		return false;
	if (B->next != A)
		return false;

	LLIST_moveBefore(A, B);
	//A->B
	if (A->prev != NULLP)
		return false;
	if (A->next != B)
		return false;
	if (B->next != NULLP)
		return false;
	if (B->prev != A)
		return false;

	//movesort tests
	LLIST_moveSort(C, A, unitTestCompare);
	//A->B->C
	if (B->next != C)
		return false;
	if (C->prev != B)
		return false;

	LLIST_moveAfter(C, NULLP);
	//A->B
	*((u8_t *)C->value) = 0;
	LLIST_moveSort(C, A, unitTestCompare);
	//C->A->B
	if (B->next != NULLP)
		return false;
	if (C->prev != NULLP)
		return false;
	if (C->next != A)
		return false;
	if (A->prev != C)
		return false;

	LLIST_moveAfter(C, NULLP);
	//A->B
	*((u8_t *)C->value) = 2;
	LLIST_moveSort(C, A, unitTestCompare);
	//A->B->C
	if (B->next != C)
		return false;
	if (C->prev != B)
		return false;

	*((u8_t *)C->value) = 3;
	//end of movesort tests

	if (LLIST_getLast(B) != C)
		return false;
	if (LLIST_getFirst(B) != A)
		return false;

	LLIST_moveToEnd(A, A);
	//B->C->A
	if (B->prev != NULLP)
		return false;
	if (C->prev != B)
		return false;
	if (A->prev != C)
		return false;
	if (B->next != C)
		return false;
	if (C->next != A)
		return false;
	if (A->next != NULLP)
		return false;

	LLIST_moveToStart(C, A);
	//C->B->A
	if (C->prev != NULLP)
		return false;
	if (C->next != B)
		return false;
	if (B->prev != C)
		return false;
	if (B->next != A)
		return false;
	if (A->prev != B)
		return false;
	if (A->next != NULLP)
		return false;

	LLIST_moveToStart(C, A);
	//C->B->A
	if (C->prev != NULLP)
		return false;
	if (C->next != B)
		return false;
	if (B->prev != C)
		return false;
	if (B->next != A)
		return false;
	if (A->prev != B)
		return false;
	if (A->next != NULLP)
		return false;

	if (LLIST_find(C, unitTestFind) != B)
		return false;

	LLIST_join(A, C);
	//C->B->A->C...
	if (A->next != C)
		return false;
	if (C->next != B)
		return false;
	if (B->next != A)
		return false;
	if (B->prev != C)
		return false;
	if (C->prev != A)
		return false;
	if (A->prev != B)
		return false;

	if (LLIST_foreach(A, unitTestIter) != 3)
		return false;
	if (*((u8_t *)A->value) != 11)
		return false;
	if (*((u8_t *)B->value) != 12)
		return false;
	if (*((u8_t *)C->value) != 13)
		return false;

	LLIST_join(A, NULLP);
	//C->B->A
	if (A->next != NULLP)
		return false;
	if (C->next != B)
		return false;
	if (B->next != A)
		return false;
	if (B->prev != C)
		return false;
	if (C->prev != NULLP)
		return false;
	if (A->prev != B)
		return false;

	LLIST_sort(C, unitTestCompare);
	//A->B->C
	if (A->prev != NULLP)
		return false;
	if (A->next != B)
		return false;
	if (B->prev != A)
		return false;
	if (B->next != C)
		return false;
	if (C->prev != B)
		return false;
	if (C->next != NULLP)
		return false;

	LLIST_Node_t *D;
	LLIST_Node_t *E;
	LLIST_Node_t *F;
	D = LLIST_new(1);
	E = LLIST_new(1);
	F = LLIST_new(1);
	if (D == NULLP)
		return false;
	if (E == NULLP)
		return false;
	if (F == NULLP)
		return false;
	*((u8_t *)D->value) = 4;
	*((u8_t *)E->value) = 5;
	*((u8_t *)F->value) = 6;

	LLIST_moveAfter(D, E);
	LLIST_moveAfter(F, E);
	//E->F->D
	if (E->next != F)
		return false;
	if (F->next != D)
		return false;

	LLIST_join(D, A);
	//E->F->D->A->B->C
	if (D->prev != F)
		return false;
	if (D->next != A)
		return false;
	if (A->prev != D)
		return false;
	if (A->next != B)
		return false;

	*Aval      = 1;
	*Bval      = 2;
	*Cval      = 3;
	u8_t *Dval = (u8_t *)D->value;
	u8_t *Eval = (u8_t *)E->value;
	u8_t *Fval = (u8_t *)F->value;
	if (*Aval != 1)
		return false;
	if (*Bval != 2)
		return false;
	if (*Cval != 3)
		return false;
	if (*Dval != 4)
		return false;
	if (*Eval != 5)
		return false;
	if (*Fval != 6)
		return false;

	LLIST_sort(D, unitTestCompare);
	//A->B->C->D->E->F
	if (A->prev != NULLP)
		return false;
	if (A->next != B)
		return false;
	if (B->prev != A)
		return false;
	if (B->next != C)
		return false;
	if (C->prev != B)
		return false;
	if (C->next != D)
		return false;
	if (D->prev != C)
		return false;
	if (D->next != E)
		return false;
	if (E->prev != D)
		return false;
	if (E->next != F)
		return false;
	if (F->prev != E)
		return false;
	if (F->next != NULLP)
		return false;

	if (LLIST_delete(B) != C)
		return false;
	//A->C->D->E->F
	if (A->next != C)
		return false;
	if (C->prev != A)
		return false;

	return true;
}

static i8_t unitTestCompare(const LLIST_Node_t *first, const LLIST_Node_t *second)
{
	u8_t *firstVal  = (u8_t *)first->value;
	u8_t *secondVal = (u8_t *)second->value;

	if (*firstVal > *secondVal)
	{
		return +1;
	}
	else if (*firstVal < *secondVal)
	{
		return -1;
	}
	else
	{
		return 0;
	}
}

static i8_t unitTestFind(LLIST_Node_t *node)
{
	return *((u8_t *)node->value) == 2;
}

static i8_t unitTestIter(LLIST_Node_t *node)
{
	static u8_t count = 0;
	if (count < 3)
	{
		count++;
		(*(u8_t *)node->value) += 10;
		return 1;
	}
	else
	{
		return -1;
	}
}

static bool isFail  = false;
static int  nextNum = 1;

static i8_t badFunc(LLIST_Node_t *node)
{
	u8_t *val = (u8_t *)node->value;
	printf("Value is: %#03x\n", *val);
	if (*val != nextNum)
	{
		isFail = true;
	}
	nextNum = 3;
	LLIST_delete(node->next);
	return 1;
}

//demonstrating error with modifying list during iteration
bool LLIST_unitTest_foreach(void)
{
	LLIST_Node_t *A;
	LLIST_Node_t *B;
	LLIST_Node_t *C;
	A = LLIST_new(1);
	B = LLIST_new(1);
	C = LLIST_new(1);
	if (A == NULLP)
		return false;
	if (B == NULLP)
		return false;
	if (C == NULLP)
		return false;
	*((u8_t *)A->value) = 1;
	*((u8_t *)B->value) = 2;
	*((u8_t *)C->value) = 3;

	u8_t *Aval = (u8_t *)A->value;
	u8_t *Bval = (u8_t *)B->value;
	u8_t *Cval = (u8_t *)C->value;
	if (*Aval != 1)
		return false;
	if (*Bval != 2)
		return false;
	if (*Cval != 3)
		return false;

	LLIST_moveAfter(B, A);
	LLIST_moveAfter(C, B);

	LLIST_foreach(A, badFunc);

	return isFail;
}
