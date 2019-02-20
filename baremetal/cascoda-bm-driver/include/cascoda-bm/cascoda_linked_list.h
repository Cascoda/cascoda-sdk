
/**
	@file cascoda_linked_list.h
	@author Andrew Howe
	@brief Dynamically allocated doubly-linked list

	Use LLIST_new and LLIST_delete to manage dynamically allocated memory of the nodes and values
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
#ifndef CASCODA_DLINKELLIST_H
#define CASCODA_DLINKELLIST_H

#include <stdbool.h>
#include "cascoda-bm/cascoda_types.h"

/******************************************************************************/
/****** Structures and callback function signatures                      ******/
/******************************************************************************/

/** Struct for a node in a linkedList
*/
typedef struct DLinkeLLIST_struct
{
	void *                     value;
	struct DLinkeLLIST_struct *prev;
	struct DLinkeLLIST_struct *next;
} LLIST_Node_t;

/** Signature for comparison function
	Must return > 0 if first > second
	Must return < 0 if first < second
	Must return 0 if first == second
*/
typedef i8_t(LLIST_Compare_func)(const LLIST_Node_t *first, const LLIST_Node_t *second);

/** Signature for iteration function
	Return status indicates "matching" to be indicated in the return value of LLIST_foreach or LLIST_find
	Return > 0 if the node "matches"
	Return 0 if the node does not "match"
	Return < 0 to abort the iteration
*/
typedef i8_t(LLIST_Iter_func)(LLIST_Node_t *node);

/******************************************************************************/
/****** API functions                                                    ******/
/******************************************************************************/

/** Create a new node, allocating memory for it and its value
	@param The size of memory allocated to the node's value pointer.
	@return a pointer to the new node
*/
LLIST_Node_t *LLIST_new(u8_t size);

/** Delete a node, freeing the memory allocated for it and its value

	A->B->C->D
	LLIST_delete(B);
	A->C->D

	@param the node to delete
	@return the next node (or null if it had none)
*/
LLIST_Node_t *LLIST_delete(LLIST_Node_t *node);

/**
	@return the last node in the list containing the given node
*/
LLIST_Node_t *LLIST_getLast(const LLIST_Node_t *node);

/**
	@return the first node in the list containing the given node
*/
LLIST_Node_t *LLIST_getFirst(const LLIST_Node_t *node);

/** Take an existing node from its current location and insert it after another node

	A->B->C->D->E
	LLIST_moveAfter(B, D);
	A->C->D->B->E

	A->B->C
	D->E->F->G
	LLIST_moveAfter(A, G);
	B->C
	D->E->F->G->A

	@param node the node to move (not NULLP)
	@param newParent the node after which to insert (may be NULLP)
*/
void LLIST_moveAfter(LLIST_Node_t *node, LLIST_Node_t *newParent);

/** Take a node from its current location and insert it before another node

	A->B->C->D->E
	LLIST_moveBefore(B, A);
	B->A->C->D->E

	A->B->C->D
	LLIST_moveBefore(B, NULLP);
	B
	A->C->D
	@param node the node to move (not NULLP)
	@param newChild the node before which to insert (may be NULLP)
*/
void LLIST_moveBefore(LLIST_Node_t *node, LLIST_Node_t *newChild);

/** Move a node to the end of a list (append)

	If the node is already at the end of the list, nothing will happen

	@param node the node to move
	@list any node in the list to append to
*/
void LLIST_moveToEnd(LLIST_Node_t *node, LLIST_Node_t *list);

/** Move a node to the start of a list (prepend)

	If the node is already at the start of the list, nothing will happen.

	@param node the node to move
	@list any node in the list to prepend to
*/
void LLIST_moveToStart(LLIST_Node_t *node, LLIST_Node_t *list);

/** Take a node from its current location and insert it into the correct place in a sorted list

	int compareInt(node* p, node* q){
		return *(int*)p->value - *(int*)q->value;
	}
	A.value = 1;
	B.value = 2;
	C.value = 3;
	A->C
	LLIST_moveSort(B, C, compareInt);
	A->B->C

	@param node the node to move
	@param list the node at which to begin comparisons to find the correct position
	@param compare the comparison function the list is sorted by
*/
void LLIST_moveSort(LLIST_Node_t *node, LLIST_Node_t *list, LLIST_Compare_func *compare);

/** Join two lists together

	The parent node will detach from its old child, and attach to the child node.
	The child node will detach from its old parent, and attach to the parent node.
	The parent node's parent and the child node's child will remain unchanged.

	A->B->C->D->E
	F->G->H->I->J
	LLIST_join(B, I);
	A->B->I->J
	C->D->E
	F->G->H

	@param parent the parent node of the join
	@param child the child node of the join
*/
void LLIST_join(LLIST_Node_t *parent, LLIST_Node_t *child);

/** Sort a list

	Sort all nodes in the given list in ascending order according to the given comparison function
	(end up with each node.next >= node)
	Doesn't work for circular lists

	@param first any node in the list to be sorted
	@param compare the comparison function by which to sort the list
	@return the new head of the list
*/
LLIST_Node_t *LLIST_sort(LLIST_Node_t *first, LLIST_Compare_func *compare);

/**	Find a particular node in a list

	@param first the node from which to begin searching down from. Nodes previous to first will not be checked.
	@param check a function which determines whether a node matches or not
	@return the first node, starting from the given node, that returns positive when passed to the given function.
	@return nullp if no such node is found.
*/
LLIST_Node_t *LLIST_find(LLIST_Node_t *first, LLIST_Iter_func *check);

/** Iterate down the list, passing each node to the given function

	THE GIVEN FUNCTION SHOULD NOT MODIFY THE LIST BEING ITERATED ON.
	A node inserted immediately after the node currently being iterated on will not be part of the iteration
	Deleting the node following the node currently being iterated on will result in a NULLP dereference.
	Moving nodes in the list will result in unexpected order of execution.

	@param first the node from which to begin the iteration. Nodes previous to first will not be iterated over.
	@param func the function to call once per node
	@return the number of nodes for which the given function returned a positive number
*/
u8_t LLIST_foreach(LLIST_Node_t *first, LLIST_Iter_func *func);

/** Delete every node in the given list
	@param list any node in the list to destroy
	@return the number of nodes deleted
*/
u8_t LLIST_destroy(LLIST_Node_t *list);

//bool LLIST_unitTest(void);
//bool LLIST_unitTest_foreach(void);

#endif
