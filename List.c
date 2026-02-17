#include "List.h"
#include <stdio.h>
#include <pthread.h>

static Node nodePool[LIST_MAX_NUM_NODES];
static List listPool[LIST_MAX_NUM_HEADS];
static unsigned int totalNodeCount = 0;
static unsigned int totalListCount = 0;
static unsigned int nodesFreed = 0;
static unsigned int listsFreed = 0;
static unsigned int unusedNodes[LIST_MAX_NUM_NODES];
static unsigned int unusedLists[LIST_MAX_NUM_HEADS];

static Node * List_create_node() {
    unsigned int unusedNodesIndex = (totalNodeCount + nodesFreed) % LIST_MAX_NUM_NODES;
    unsigned int nodePoolIndex = unusedNodes[unusedNodesIndex];
    Node * newNode = &nodePool[nodePoolIndex];
    totalNodeCount++;
    return newNode;
}

static List * List_create_helper() {
    unsigned int unusedListsIndex = (totalListCount + listsFreed) % LIST_MAX_NUM_HEADS;
    unsigned int listPoolIndex = unusedLists[unusedListsIndex];
    List * newList = &listPool[listPoolIndex];
    newList->current = NULL;
    newList->head = NULL;
    newList->tail = NULL;
    newList->itemCount = 0;
    newList->oob = LIST_OOB_START;
    totalListCount++;
    return newList;
}

static void List_free_node(Node * node) {
    unsigned int unusedNodesIndex = node->index;
    node->item = NULL;
    node->next = NULL;
    node->prev = NULL;
    node = NULL;
    unusedNodes[nodesFreed % LIST_MAX_NUM_NODES] = unusedNodesIndex;
    nodesFreed++;
    totalNodeCount--;
}

static void List_free_helper(List * list) {
    unsigned int unusedListsIndex = list->index;
    list->current = NULL;
    list->head = NULL;
    list->tail = NULL;
    list->itemCount = 0;
    list = NULL;
    unusedLists[listsFreed % LIST_MAX_NUM_HEADS] = unusedListsIndex;
    listsFreed++;
    totalListCount--;
}

static void List_insert_into_empty(List * pList, void * item) {
    Node * newNode = List_create_node();
    pList->current = newNode;
    pList->current->item = item;
    pList->head = pList->current;
    pList->tail = pList->current;
    pList->itemCount++;
}

static void List_print(List * pList) {
    Node * temp = pList->head;
    printf("Current contents: ");
    while (temp != NULL) {
        printf("%p ", temp->item);
        temp = temp->next;
    }
    temp = NULL;
    printf("\n");
    printf("Total nodes used: %d\n", totalNodeCount);
}

List* List_create() {
    if ((totalListCount + listsFreed) == 0) {
        for (int i = 0; i < LIST_MAX_NUM_NODES; i++) {
            unusedNodes[i] = i;
            nodePool[i].index = i;
        }
        for (int i = 0; i < LIST_MAX_NUM_HEADS; i++) {
            unusedLists[i] = i;
            listPool[i].index = i;
        }
    }
    if (totalListCount < LIST_MAX_NUM_HEADS) {
        List * newList = List_create_helper();
        return newList;
    }
    return NULL;
}

int List_count(List* pList) {
    return pList->itemCount;
}

void* List_first(List* pList) {
    if (pList->itemCount == 0) {
        pList->current = NULL;
        return NULL;
    }
    pList->current = pList->head;
    return pList->current->item;
}

void* List_last(List* pList) {
    if (pList->itemCount == 0) {
        pList->current = NULL;
        return NULL;
    }
    pList->current = pList->tail;
    return pList->current->item;
}

void* List_next(List* pList) {
    if (pList->itemCount == 0) {
        pList->oob = LIST_OOB_END;
        return NULL;
    }
    if (pList->current == pList->tail || (pList->current == NULL && pList->oob == LIST_OOB_END) ) {
        pList->oob = LIST_OOB_END;
        pList->current = NULL;
        return NULL;
    }
    if (pList->current == NULL && pList->oob == LIST_OOB_START) {
        pList->current = pList->head;
        return pList->current->item;
    }
    pList->current = pList->current->next;
    return pList->current->item;
}

void* List_prev(List* pList) {
    if (pList->itemCount == 0) {
        pList->oob = LIST_OOB_START;
        return NULL;
    }
    if (pList->current == pList->head || (pList->current == NULL && pList->oob == LIST_OOB_START) ) {
        pList->oob = LIST_OOB_START;
        pList->current = NULL;
        return NULL;
    }
    if (pList->current == NULL && pList->oob == LIST_OOB_END) {
        pList->current = pList->tail;
        return pList->current->item;
    }
    pList->current = pList->current->prev;
    return pList->current->item;
}

void* List_curr(List* pList) {
    if (pList->current == NULL || pList->itemCount == 0)
        return NULL;
    return pList->current->item;
}

int List_insert_after(List* pList, void* pItem) {
    if (totalNodeCount == LIST_MAX_NUM_NODES)
        return LIST_FAIL;
    if (pList->itemCount == 0) {
        List_insert_into_empty(pList, pItem);
        return LIST_SUCCESS;
    }
    Node * newNode = List_create_node();
    if (pList->current == pList->tail || (pList->current == NULL && pList->oob == LIST_OOB_END) ) {
        pList->current = newNode;
        pList->current->item = pItem;
        pList->current->next = NULL;
        pList->current->prev = pList->tail;
        pList->tail->next = pList->current;
        pList->tail = pList->current;
    }
    else if (pList->current == NULL && pList->oob == LIST_OOB_START) {
        pList->current = newNode;
        pList->current->item = pItem;
        pList->current->next = pList->head;
        pList->current->prev = NULL;
        pList->head->prev = pList->current;
        pList->head = pList->current;
    }
    else {
        Node * temp = pList->current;
        pList->current = newNode;
        pList->current->item = pItem;
        pList->current->next = temp->next;
        pList->current->prev = temp;
        temp->next->prev = pList->current;
        temp->next = pList->current;
        temp = NULL;
    }
    pList->itemCount++;
    return LIST_SUCCESS;
}

int List_insert_before(List* pList, void* pItem) {
    if (totalNodeCount == LIST_MAX_NUM_NODES)
        return LIST_FAIL;
    if (pList->itemCount == 0) {
        List_insert_into_empty(pList, pItem);
        return LIST_SUCCESS;
    }
    Node * newNode = List_create_node();
    if (pList->current == pList->head || (pList->current == NULL && pList->oob == LIST_OOB_START) ) {
        pList->current = newNode;
        pList->current->item = pItem;
        pList->current->next = pList->head;
        pList->current->prev = NULL;
        pList->head->prev = pList->current;
        pList->head = pList->current;
    }
    else if (pList->current == NULL && pList->oob == LIST_OOB_END) {
        pList->current = newNode;
        pList->current->item = pItem;
        pList->current->next = NULL;
        pList->current->prev = pList->tail;
        pList->tail->next = pList->current;
        pList->tail = pList->current;
    }
    else {
        Node * temp = pList->current;
        Node * tempPrev = pList->current->prev;
        pList->current = newNode;
        pList->current->item = pItem;
        pList->current->next = temp;
        pList->current->prev = temp->prev;
        temp->prev->next = pList->current;
        temp->prev = pList->current;
        temp = NULL;
    }
    pList->itemCount++;
    return LIST_SUCCESS;
}

int List_append(List* pList, void* pItem) {
    if (totalNodeCount == LIST_MAX_NUM_NODES) {
        printf("Error: Max process limit reached\n");
        return LIST_FAIL;
    }
    if (pList->itemCount == 0) {
        List_insert_into_empty(pList, pItem);
        return LIST_SUCCESS;
    }
    Node * newNode = List_create_node();
    pList->current = newNode;
    pList->current->item = pItem;
    pList->current->next = NULL;
    pList->current->prev = pList->tail;
    pList->tail->next = pList->current;
    pList->tail = pList->current;
    pList->itemCount++;
    return LIST_SUCCESS;
}

int List_prepend(List* pList, void* pItem) {
    if (totalNodeCount == LIST_MAX_NUM_NODES)
        return LIST_FAIL;
    if (pList->itemCount == 0) {
        List_insert_into_empty(pList, pItem);
        return LIST_SUCCESS;
    }
    Node * newNode = List_create_node();
    pList->current = newNode;
    pList->current->item = pItem;
    pList->current->next = pList->head;
    pList->current->prev = NULL;
    pList->head->prev = pList->current;
    pList->head = pList->current;
    pList->itemCount++;
    return LIST_SUCCESS;
}

void* List_remove(List* pList) {
    if (pList->itemCount == 0 || pList->current == NULL) {
        return NULL;
    }
    void * tempItem = pList->current->item;
    if (pList->itemCount == 1) {
        pList->head = NULL;
        pList->tail = NULL;
        List_free_node(pList->current);
    }
    else if (pList->current == pList->head) {
        pList->head = pList->head->next;
        pList->head->prev = NULL;
        List_free_node(pList->current);
        pList->current = pList->head;
    }
    else if (pList->current == pList->tail) {
        pList->tail = pList->tail->prev;
        pList->tail->next = NULL;
        List_free_node(pList->current);
        pList->oob = LIST_OOB_END;
        pList->current = NULL;
    }
    else {
        Node * tempNext = pList->current->next;
        Node * tempPrev = pList->current->prev;
        tempPrev->next = tempNext;
        List_free_node(pList->current);
        pList->current = tempNext;
        pList->current->prev = tempPrev;
    }
    pList->itemCount--;
    return tempItem;
}

void* List_trim(List* pList) {
    if (pList->itemCount == 0)
        return NULL;
    void * tempItem = pList->tail->item;
    pList->current = pList->tail;
    if (pList->itemCount == 1) {
        pList->head = NULL;
        pList->tail = NULL;
        List_free_node(pList->current);
    }
    else {
        pList->tail = pList->tail->prev;
        pList->tail->next = NULL;
        List_free_node(pList->current);
        pList->current = pList->tail;
    }
    pList->itemCount--;
    return tempItem;
}

void List_concat(List* pList1, List* pList2) {
    if (pList1->itemCount == 0) {
        pList1->current = pList2->current;
        pList1->head = pList2->head;
        pList1->tail = pList2->tail;
        pList1->itemCount = pList2->itemCount;
        pList1->oob = pList2->oob;
        pList1 = pList2;
        List_free_helper(pList2);
        return;
    }
    if (pList2->itemCount == 0) {
        List_free_helper(pList2);
        return;
    }
    pList1->tail->next = pList2->head;
    pList2->head->prev = pList1->tail;
    pList1->tail = pList2->tail;
    pList1->itemCount += pList2->itemCount;
    List_free_helper(pList2);
}

typedef void (*FREE_FN)(void* pItem);
void List_free(List* pList, FREE_FN pItemFreeFn) {
    if (pList->itemCount != 0) {
        pList->current = pList->head;
        while (pList->current != NULL) {
            (*pItemFreeFn)(pList->current->item);
            List_remove(pList);
            pList->current = pList->head;
        }
    }
    List_free_helper(pList);
}

typedef bool (*COMPARATOR_FN)(void* pItem, void* pComparisonArg);
void* List_search(List* pList, COMPARATOR_FN pComparator, void* pComparisonArg) {
    List_first(pList);
    if (pList->current == NULL && pList->oob == LIST_OOB_END)
        return NULL;
    if (pList->current == NULL && pList->oob == LIST_OOB_START) {
        pList->current = pList->head;
    }
    while (pList->current != NULL) {
        printf("Comparing...\n");
        if (pComparator(pList->current->item, pComparisonArg) == true)
            return pList->current->item;
        pList->current = pList->current->next;
    }
    pList->oob = LIST_OOB_END;
    return NULL;
}
