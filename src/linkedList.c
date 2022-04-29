#include "linkedList.h"
#include <unistd.h>

struct Node **newList() {
    struct Node **head = (struct Node **) malloc(sizeof(struct Node *));
    *head = NULL;
    return head;
}

int push(struct Node **head, int idx, void *data, size_t size) {
    /* create a new Node, and fill it in */
    struct Node *new_node = (struct Node *) malloc(sizeof(struct Node));
    new_node->data = malloc(size);
    char *datap = (char *) data;
    char *node_datap = (char *) new_node->data;
    for (unsigned long i = 0; i < size; ++i)
        *node_datap++ = *datap++;

    /* connect the new Node to the list */
    struct Node *iterator = *head;
    if (idx < -1)
        return 1;
    /* -1: add to the end */
    if (idx == -1) {
        new_node->next = NULL;
        if (length(head) == 0)  
            *head = new_node;
        while (iterator) {
            if (iterator->next == NULL) 
                break;
            iterator = iterator->next;
        }
        iterator->next = new_node;
        return 0;
    }
    int scanner = 0;
    struct Node *shadow = NULL;
    /* idx: find the element to idx, and insert in front of that element */
    while (iterator && scanner < idx) {
        shadow = iterator;
        iterator = iterator->next;
        scanner ++;
    }
    if (scanner < idx)
        return 1;
    new_node->next = iterator;
    if (shadow == NULL) {
        *head = new_node;
    } else
        shadow->next = new_node;
    return 0;
}

int erase(struct Node **head, int idx) {
    struct Node *iterator = *head;
    if (idx < -1)
        return 1;
    struct Node *shadow = NULL;
    /* -1: erase the last element */
    if (idx == -1) {
        if (length(head) == 0)
            return 1;
        while (iterator) {
            if (iterator->next == NULL) 
                break;
            shadow = iterator;
            iterator = iterator->next;
        }
        free(iterator->data);
        free(iterator);
        if (shadow == NULL)
            *head = NULL;
        else
            shadow->next = NULL;
        return 0;
    }
    /* idx: find the element to idx, and erase that element */
    int scanner = 0;
    while (scanner < idx) {
        shadow = iterator;
        iterator = iterator->next;
        scanner ++;
        if (iterator == NULL)
            return 1;
    }
    if (shadow == NULL)
        *head = iterator->next;
    else
        shadow->next = iterator->next;
    free(iterator->data);
    free(iterator);
    return 0;
}

void print(struct Node **head, void (*print)(void *)) {
    if (*head == NULL)
        return;
    struct Node *iterator = *head;
    while (iterator) {
        (*print)(iterator->data);
        iterator = iterator->next;
    }
}

void clear(struct Node **head, void (*clean)(void *)) {
    while (*head) {
        if (clean != NULL)
            clean(get(head, 0));
        erase(head, 0);
    }
    free(head);
}

int length(struct Node **head) {
    if (*head == NULL)  return 0;
    struct Node *iterator = *head;
    int count = 0;
    while (iterator) {
        iterator = iterator->next;
        count ++;
    }
    return count;
}

void *get(struct Node **head, int idx) {
    struct Node *iterator = *head;
    if (idx < -1)
        return NULL;
    /* -1: return the last data */
    if (idx == -1) {
        if (length(head) == 0)
            return NULL;
        while (iterator) {
            if (iterator->next == NULL) 
                break;
            iterator = iterator->next;
        }
        return iterator->data;
    }
    /* idx: find the data to idx, and return that data */
    int scanner = 0;
    while (scanner < idx) {
        iterator = iterator->next;
        scanner ++;
        if (iterator == NULL)
            return NULL;
    }
    return iterator->data;
}