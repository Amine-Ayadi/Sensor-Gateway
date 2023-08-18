/**
 * \author Amine Ayadi
 */

#ifndef _DPLIST_H_
#define _DPLIST_H_

typedef void* element_t;

typedef enum {false, true} bool;

/**
 * dplist_t is a struct containing at least a head pointer to the start of the list;
 */
typedef struct dplist dplist_t;

typedef struct dplist_node dplist_node_t;

typedef void* element_t;

struct dplist_node {
    dplist_node_t *prev, *next;
    element_t element; // make sure to free the element first, then the node
};

struct dplist {
    dplist_node_t *head;
    // more fields will be added later
    void* (*element_copy_callback)(void *x);
    void (*element_free_callback)(void **element);
    int (*element_compare_callback)(void *x, void* y);
};

/* General remark on error handling
 * All functions below will:
 * - use assert() to check if memory allocation was successfully.
 */

/** Create and allocate memory for a new list
 * \return a pointer to a newly-allocated and initialized list.
 */
dplist_t *dpl_create(
    void* (*element_copy)(void *x),
    void (*element_free)(void **element),
    int (*element_compare)(void *x, void *y));

/** Deletes all elements in the list
 * - Every list node of the list needs to be deleted. (free memory)
 * - The list itself also needs to be deleted. (free all memory)
 * - '*list' must be set to NULL.
 * \param list a double pointer to the list
 */
void dpl_free(dplist_t **list, bool free_element);

/** Returns the number of elements in the list.
 * - If 'list' is is NULL, -1 is returned.
 * \param list a pointer to the list
 * \return the size of the list
 */
int dpl_size(dplist_t *list);

/** Inserts a new list node containing an 'element' in the list at position 'index'
 * - the first list node has index 0.
 * - If 'index' is 0 or negative, the list node is inserted at the start of 'list'.
 * - If 'index' is bigger than the number of elements in the list, the list node is inserted at the end of the list.
 * - If 'list' is is NULL, NULL is returned.
 * \param list a pointer to the list
 * \param element the data that needs to be inserted
 * \param index the position at which the element should be inserted in the list
 * \return a pointer to the list or NULL
 */
dplist_t *dpl_insert_at_index(dplist_t *list, element_t element, int index, bool insert_copy);

/** Removes the list node at index 'index' from the list.
 * - The list node itself should always be freed.
 * - If 'index' is 0 or negative, the first list node is removed.
 * - If 'index' is bigger than the number of elements in the list, the last list node is removed.
 * - If the list is empty, return the unmodified list.
 * - If 'list' is is NULL, NULL is returned.
 * \param list a pointer to the list
 * \param index the position at which the node should be removed from the list
 * \return a pointer to the list or NULL
 */

dplist_t *dpl_remove_at_index(dplist_t *list, int index, bool free_element);

/** Returns a reference to the list node with index 'index' in the list.
 * - If 'index' is 0 or negative, a reference to the first list node is returned.
 * - If 'index' is bigger than the number of list nodes in the list, a reference to the last list node is returned.
 * - If the list is empty, NULL is returned.
 * - If 'list' is is NULL, NULL is returned.
 * \param list a pointer to the list
 * \param index the position of the node for which the reference is returned
 * \return a pointer to the list node at the given index or NULL
 */
dplist_node_t *dpl_get_reference_at_index(dplist_t *list, int index);

/** Returns the list element contained in the list node with index 'index' in the list.
 * - return is not returning a copy of the element with index 'index', i.e. 'element_copy()' is not used.
 * - If 'index' is 0 or negative, the element of the first list node is returned.
 * - If 'index' is bigger than the number of elements in the list, the element of the last list node is returned.
 * - If the list is empty, 0 is returned.
 * - If 'list' is NULL, 0 is returned.
 * \param list a pointer to the list
 * \param index the position of the node for which the element is returned
 * \return the element at the given index
 */
element_t dpl_get_element_at_index(dplist_t *list, int index);

/** Returns an index to the first list node in the list containing 'element'.
 * - the first list node has index 0.
 * - If 'element' is not found in the list, -1 is returned.
 * - If 'list' is NULL, -1 is returned.
 * \param list a pointer to the list
 * \param element the element to look for
 * \return the index of the element that matches 'element'
 */
int dpl_get_index_of_element(dplist_t *list, element_t element);

/** Returns the element contained in the list node with reference 'reference' in the list.
 * - If the list is empty, NULL is returned.
 * - If 'list' is is NULL, NULL is returned.
 * - If 'reference' is NULL, NULL is returned.
 * - If 'reference' is not an existing reference in the list, NULL is returned.
 * \param list a pointer to the list
 * \param reference a pointer to a certain node in the list
 * \return the element contained in the list node or NULL
 */
void *dpl_get_element_at_reference(dplist_t *list, dplist_node_t *reference);


//*** HERE STARTS THE EXTRA SET OF OPERATORS ***//


/** Returns a reference to the first list node of the list.
 * - If the list is empty, NULL is returned.
 * - If 'list' is is NULL, NULL is returned.
 * \param list a pointer to the list
 * \return a reference to the first list node of the list or NULL
 */
dplist_node_t *dpl_get_first_reference(dplist_t *list);

/** Returns a reference to the last list node of the list.
 * - If the list is empty, NULL is returned.
 * - If 'list' is is NULL, NULL is returned.
 * \param list a pointer to the list
 * \return a reference to the last list node of the list or NULL
 */
dplist_node_t *dpl_get_last_reference(dplist_t *list);

/** Returns a reference to the next list node of the list node with reference 'reference' in the list.
 * - If the list is empty, NULL is returned.
 * - If 'list' is is NULL, NULL is returned.
 * - If 'reference' is NULL, NULL is returned.
 * - If 'reference' is not an existing reference in the list, NULL is returned.
 * \param list a pointer to the list
 * \param reference a pointer to a certain node in the list
 * \return a pointer to the node next to 'reference' in the list or NULL
 */
dplist_node_t *dpl_get_next_reference(dplist_t *list, dplist_node_t *reference);

/** Returns a reference to the previous list node of the list node with reference 'reference' in 'list'.
 * - If the list is empty, NULL is returned.
 * - If 'list' is is NULL, NULL is returned.
 * - If 'reference' is NULL, NULL is returned.
 * - If 'reference' is not an existing reference in the list, NULL is returned.
 * \param list a pointer to the list
 * \param reference a pointer to a certain node in the list
 * \return pointer to the node previous to 'reference' in the list or NULL
 */
dplist_node_t *dpl_get_previous_reference(dplist_t *list, dplist_node_t *reference);

/** Returns a reference to the first list node in the list containing 'element'.
 * - If the list is empty, NULL is returned.
 * - If 'list' is is NULL, NULL is returned.
 * - If 'element' is not found in the list, NULL is returned.
 * \param list a pointer to the list
 * \param element a pointer to an element
 * \return the first list node in the list containing 'element' or NULL
 */
dplist_node_t *dpl_get_reference_of_element(dplist_t *list, void *element);

/** Returns the index of the list node in the list with reference 'reference'.
 * - the first list node has index 0.
 * - If the list is empty, -1 is returned.
 * - If 'list' is is NULL, -1 is returned.
 * - If 'reference' is NULL, -1 returned.
 * - If 'reference' is not an existing reference in the list, -1 is returned.
 * \param list a pointer to the list
 * \param reference a pointer to a certain node in the list
 * \return the index of the given reference in the list
 */
int dpl_get_index_of_reference(dplist_t *list, dplist_node_t *reference);

/** Inserts a new list node containing an 'element' in the list at position 'reference'.
 * - If 'list' is is NULL, NULL is returned.
 * - If 'reference' is NULL, NULL is returned (nothing is inserted).
 * - If 'reference' is not an existing reference in the list, 'list' is returned (nothing is inserted).
 * \param list a pointer to the list
 * \param element a pointer to an element
 * \param reference a pointer to a certain node in the list
 * \param insert_copy if true use element_copy() to make a copy of 'element' and use the copy in the new list node, otherwise the given element pointer is added to the list
 * \return a pointer to the list or NULL
 */
//TODO:
dplist_t *dpl_insert_at_reference(dplist_t *list, void *element, dplist_node_t *reference, bool insert_copy); //

/** Inserts a new list node containing 'element' in the sorted list and returns a pointer to the new list.
 * - The list must be sorted or empty before calling this function.
 * - The sorting is done in ascending order according to a comparison function.
 * - If two members compare as equal, their order in the sorted array is undefined.
 * - If 'list' is is NULL, NULL is returned.
 * \param list a pointer to the list
 * \param element a pointer to an element
 * \param insert_copy if true use element_copy() to make a copy of 'element' and use the copy in the new list node, otherwise the given element pointer is added to the list
 * \return a pointer to the list or NULL
 */
//TODO:
dplist_t *dpl_insert_sorted(dplist_t *list, void *element, bool insert_copy);//, int (*element_compare)(void *x, void *y)); // 

/** Removes the list node with reference 'reference' in the list.
 * - The list node itself should always be freed.
 * - If 'reference' is NULL, NULL is returned (nothing is removed).
 * - If 'reference' is not an existing reference in the list, 'list' is returned (nothing is removed).
 * - If 'list' is is NULL, NULL is returned.
 * \param list a pointer to the list
 * \param reference a pointer to a certain node in the list
 * \param free_element if true call element_free() on the element of the list node to remove
 * \return a pointer to the list or NULL
 */
dplist_t *dpl_remove_at_reference(dplist_t *list, dplist_node_t *reference, bool free_element);

/** Finds the first list node in the list that contains 'element' and removes the list node from 'list'.
 * - If 'element' is not found in 'list', the unmodified 'list' is returned.
 * - If 'list' is is NULL, NULL is returned.
 * \param list a pointer to the list
 * \param element a pointer to an element
 * \param free_element if true call element_free() on the element of the list node to remove
 * \return a pointer to the list or NULL
 */
dplist_t *dpl_remove_element(dplist_t *list, void *element, bool free_element);
#endif  //DPLIST_H_
