#ifndef LINKEDLIST_H
#define LINKEDLIST_H

#include <stdlib.h>

////////////////////////////////////////////////////////
///                  data structure                  ///
////////////////////////////////////////////////////////

struct Node {
    void *data;
    struct Node *next;
};

typedef struct Node node_t;

/////////////////////////////////////////////////////////
///                      methods                      ///
/////////////////////////////////////////////////////////

/* return an empty list, which is a [Node **] pointer pointing to a
        [Node *], which is NULL
 */
struct Node **newList();

/* add [data] to the [idx]th position in the linkedlist. 
 * a dynamically allocated memory would be created for [*data]
 * if [idx] = 0, add to the front. 
 * if [idx] = length of linkedlist, add to the end
 * if [idx] = -1, add to the end
 * if [idx] > length of linkedlist or < -1, error
 * if error, return 1, otherwise 0
 */
int push(struct Node **head, int idx, void *data, size_t size);

/* erase the element in the linkedlist at the position [idx]
 * if the list is empty, error
 * the dynamically allocated memory for that element would be freed
 * if [idx] = 0, erase the first element
 * if [idx] = length of linkedlist - 1, erase the last element
 * if [idx] = -1, erase the last element
 * if [idx] >= length of linkedlist or < -1, error
 * if error, return 1, otherwise 0
 */
int erase(struct Node **head, int idx);

/* print the linkedlist
 * [print] is a function pointer that specifies how to print
        an individual element
 */
void print(struct Node **head, void (*print)(void *));

/* free the memories occupied by linkedlist
 * [clean] is a function pointer that specifies how to free
        the memory allocated inside each data type.
        if [clean] = NULL, then nothing is done to the individual
        element
 */
void clear(struct Node **head, void (*clean)(void *));

/* calculate the length of the list */
int length(struct Node **head);

/* get the pointer of the element at the position [idx]
 * if [idx] = 0, return the pointer to the first element
 * if [idx] = length of linkedlist - 1, return the last element's pointer
 * if [idx] = -1, return the pointer of the last element
 * if [idx] >= length of linkedlist or < -1, error
 * if err, return NULL
 */
void *get(struct Node **head, int idx);

#endif