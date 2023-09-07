/**
 * \author Amine Ayadi
 */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "dplist.h"
#include <string.h>

//#define DEBUG

/*
 * definition of error codes
 */
#define DPLIST_NO_ERROR 0
#define DPLIST_MEMORY_ERROR 1   //error due to mem alloc failure
#define DPLIST_INVALID_ERROR 2  //error due to a list operation applied on a NULL list

#ifdef DEBUG
#define DEBUG_PRINTF(...) 									                                        \
        do {											                                            \
            fprintf(stderr,"\nIn %s - function %s at line %d: ", __FILE__, __func__, __LINE__);	    \
            fprintf(stderr,__VA_ARGS__);								                            \
            fflush(stderr);                                                                         \
                } while(0)
#else
#define DEBUG_PRINTF(...) (void)0
#endif

// typedef void* element_t;

// struct dplist_node {
//     dplist_node_t *prev, *next;
//     element_t element; // make sure to free the element first, then the node
// };

// struct dplist {
//     dplist_node_t *head;
//     // more fields will be added later
//     void* (*element_copy_callback)(void *x);
//     void (*element_free_callback)(void **element);
//     int (*element_compare_callback)(void *x, void* y);
// };

#define DPLIST_ERR_HANDLER(condition, err_code)                         \
    do {                                                                \
            if ((condition)) DEBUG_PRINTF(#condition " failed\n");      \
            assert(!(condition));                                       \
        } while(0)

int dpl_get_index_of_reference(dplist_t *list, dplist_node_t *reference);

void* element_copy_string(void* element) {
    // check for null pointer
    int stringLengh = strlen((const char*)element);
    char* copy = (char*)malloc(stringLengh * sizeof(char));

    strcpy(copy, (char*)element);

    return (void*)copy;
}

void element_free_string(void **element) {
    // check pointer
    free(*element);
}

int element_compare_string(void *x, void *y) {
    // pointer check
    return strcmp (
        (const char*) x, (const char*) y);
}

dplist_t* dpl_create(
    void* (*element_copy)(void *x),
    void (*element_free)(void **element),
    int (*element_compare)(void *x, void *y)) {
    dplist_t* list = NULL;
    list = malloc(sizeof(struct dplist));
    DPLIST_ERR_HANDLER(list == NULL, DPLIST_MEMORY_ERROR);
    list->head = NULL;

    list->element_copy_callback = element_copy;
    list->element_free_callback = element_free;
    list->element_compare_callback = element_compare;
    return list;
}

void dpl_free(dplist_t **list, bool free_element) {
    //TODO : add your code here
    //DPLIST_ERR_HANDLER(list==NULL, DPLIST_INVALID_ERROR);
    //DPLIST_ERR_HANDLER(*list==NULL, DPLIST_INVALID_ERROR);
    if (!list) return;
    if (!(*list)) return;
    if ((*list)->head == NULL) return;
    dplist_node_t* list_node, * next_node;
    dplist_t* temp_list = *list;
    list_node = temp_list->head;

    while(list_node != NULL) {
        next_node =  list_node->next;
        if(free_element) (*list)->element_free_callback(&(list_node->element));
        free(list_node);
        list_node = next_node;
    }
    free(*list);
    temp_list->head = NULL;
    *list = NULL;
}

int dpl_size(dplist_t *list){
    // TODO : add your code here
    //DPLIST_ERR_HANDLER(list==NULL,DPLIST_INVALID_ERROR);
    dplist_node_t* list_node = list->head;
    int count = 0;
    if(!list) return 0;
    while(list_node != NULL){
        count++;
        list_node = list_node->next;
    }
    return count;
}

dplist_node_t* dpl_get_last_reference(dplist_t* list){
	//DPLIST_ERR_HANDLER(list==NULL,DPLIST_MEMORY_ERROR);
	dplist_node_t* list_node = list->head;
    if(list->head == NULL) return NULL;
    if(!list) return NULL;
	while(list_node->next != NULL) {
		list_node=list_node->next;
	}
	//printf(list_node->element);
    return list_node;
}

dplist_node_t* dpl_get_first_reference(dplist_t* list){
	//DPLIST_ERR_HANDLER(list==NULL,DPLIST_MEMORY_ERROR);
	dplist_node_t* list_node = list->head;
    if(!list_node) return NULL;
    if(!list) return NULL;

	//printf(list_node->element);
    return list_node;
}

dplist_node_t *dpl_get_next_reference(dplist_t *list, dplist_node_t *reference){
	//DPLIST_ERR_HANDLER(list==NULL,DPLIST_MEMORY_ERROR);
	dplist_node_t* list_node = list->head;
    //dplist_node_t* ref = reference;
    int count;
    if(!list_node) return NULL;
    if(!list) return NULL;
    for (list_node = list->head, count = 0; list_node->next != reference; list_node = list_node->next, count++){
        if (list_node->next == reference) return list_node->next;
    }
	//printf(ref->next->element);
    return list_node->next;
}

dplist_node_t *dpl_get_previous_reference(dplist_t *list, dplist_node_t *reference){
	//DPLIST_ERR_HANDLER(list==NULL,DPLIST_MEMORY_ERROR);
	dplist_node_t* list_node = list->head;
    dplist_node_t* ref = reference;
    //printf("adad\n");
    if(!list_node) return NULL;
    //printf("adad\n");
    if(!list) return NULL;
    int count;
    for (list_node = list->head, count = 0; list_node->next != ref; list_node = list_node->next, count++){
        if (dpl_get_reference_at_index(list,count) == ref) return list_node->prev;
        //printf("adad\n");
    }
	//printf(list_node->element);
    return list_node;
}

dplist_t *dpl_insert_at_index(dplist_t *list, element_t element, int index, bool insert_copy) {
    dplist_node_t *ref_at_index, *list_node;
    if (list == NULL) return NULL;

    list_node = malloc(sizeof(dplist_node_t));
    DPLIST_ERR_HANDLER(list_node == NULL, DPLIST_MEMORY_ERROR);
    if(insert_copy) list_node->element = list->element_copy_callback(element); 
    else list_node->element = element;
    if (list->head == NULL){
        list_node->prev = NULL;
        list_node->next = NULL;
        list->head = list_node;
    } else if (index <= 0){
        list_node->prev = NULL;
        list_node->next = list->head;
        list->head->prev = list_node;
        list->head = list_node;
    } else{
        ref_at_index = dpl_get_reference_at_index(list, index);
        assert(ref_at_index != NULL);
        if(index < dpl_size(list)){
            list_node->prev = ref_at_index->prev;
            list_node->next = ref_at_index;
            ref_at_index->prev->next = list_node;
            ref_at_index->prev = list_node;     
        } else{ 
            assert(ref_at_index != NULL);
            list_node->next = NULL;
            list_node->prev = ref_at_index;
            ref_at_index->next = list_node;
        }
    }
    return list;
}

void dpl_print(dplist_t *list) {
    if(!list){
        printf("NULL LIST \n");
        return;
    }
    int counter = 0;
    dplist_node_t* tmp = list->head;

    printf ("SIZE NOW IS %d\n", dpl_size(list));

    if (!tmp) {
        printf ("Head pointing to NULL\n");
        return;
    }
    counter = 1;
    while(tmp){
        printf("[%d] %s\n",counter++, (char*)tmp->element);
        tmp = tmp->next;
    }
}

#if 0
dplist_t* dpl_remove_at_index( dplist_t * list, int index, bool free_element) {
    dplist_node_t* head = list->head;
    dplist_node_t* temp = head;

    if (index == 1) {
        //printf("Treating index = 1\n");
        head = temp->next;
        (temp->next)->prev = head;
        //free(temp);
        return list;
    }

    if (index == 0) {
        head = temp->next;
        (temp->next)->prev = NULL;
        //free(temp);
        return list;    

    }

    for (int i = 0; i < index - 1; i++) {
        //printf("Treating index = %d\n", i);
        temp = temp->next;
    }

    if (temp->next != NULL) {
        printf("Treating index = %d\n", 17);
        (temp->next)->prev = temp->prev;
        (temp->prev)->next = temp->next;
    } else {
        printf("Treating index = 87\n");
        (temp->prev)->next = NULL;
    }

    //free(temp);
    return list;
}
#endif

dplist_t * dpl_remove_at_index( dplist_t * list, int index, bool free_element)
{
	dplist_node_t *ref_at_index; //, *list_node;
	//DPLIST_ERR_HANDLER(list==NULL,DPLIST_INVALID_ERROR);
    if(!list) return NULL;
	if(dpl_size(list)<=0){
		return list;
	}
	if (index <= 0)
	{
		if(free_element){
            ref_at_index = dpl_get_reference_at_index(list,index);
		    list->head = ref_at_index->next;
			list->element_free_callback(&(ref_at_index->element));
		} else return list;

		if(list->head != NULL){
			list->head->prev = NULL;
		} return list;
		free(ref_at_index);
	}else{
        if(free_element){
		    ref_at_index = dpl_get_reference_at_index(list, index); // select the right node at the right index, gives automatically the last one if the index is to big
		    assert( ref_at_index != NULL);
		    list->element_free_callback(&(ref_at_index->element));
		} else return list;
		if (index >= dpl_size(list)-1){ //TODO: -1
			if(list->head->next==NULL){
				//only one item in list
		        list->head=NULL;
			    free(ref_at_index);
		  }else{
			 // remove last node
			 ref_at_index->prev->next = NULL;
			 free(ref_at_index);
			}
		}else{
			//remove node in the middle
			ref_at_index->prev->next = ref_at_index->next;
			ref_at_index->next->prev = ref_at_index->prev; //this is wrong
			free(ref_at_index);
		}
	}
return list;
}

dplist_node_t *dpl_get_reference_at_index(dplist_t *list, int index) {
    int count;
    dplist_node_t *dummy;
    //DPLIST_ERR_HANDLER(list == NULL, DPLIST_INVALID_ERROR);
    if(!list) return NULL;
    if (list->head == NULL) return NULL;
    for (dummy = list->head, count = 0; dummy->next != NULL; dummy = dummy->next, count++) {
        if (count >= index) return dummy;
    }
    return dummy;
}

element_t dpl_get_element_at_index( dplist_t * list, int index )
{
	//DPLIST_ERR_HANDLER(list==NULL,DPLIST_INVALID_ERROR);
    if(!list) return NULL;
    if (list->head == NULL) return NULL;
	dplist_node_t* dummy;
  int count;

  for (dummy = list->head, count = 0; dummy->next != NULL ; dummy = dummy->next, count++)
  {
      if (count >= index) return dummy->element;
  }
  return dummy->element;
}

int dpl_get_index_of_element(dplist_t *list, element_t element) {

    //TODO: add your code here 
    //DPLIST_ERR_HANDLER(list == NULL, DPLIST_INVALID_ERROR);
    if (!list) { // NULL
        return -1;
    }
    dplist_node_t *list_node = list->head;
    if (list_node == NULL) {
        return -1;
    }

    int counter = 0;

    dplist_node_t* tmp = list->head;
    while (tmp) {
        if (element == tmp->element) return counter;
        tmp = tmp->next;
        counter++;
    }
    return -1;
}

dplist_node_t *dpl_get_reference_of_element(dplist_t *list, void *element){
    //DPLIST_ERR_HANDLER(list == NULL, DPLIST_INVALID_ERROR);
    if (!list) { // NULL
        return NULL;
    }
    if (list->head == NULL) return NULL;
    element_t elem = element;
    int index = dpl_get_index_of_element(list, elem);
    printf("index is %d \n",index);
    return dpl_get_reference_at_index(list, index);
}

int dpl_get_index_of_reference(dplist_t *list, dplist_node_t *reference){
    //DPLIST_ERR_HANDLER(list == NULL, DPLIST_INVALID_ERROR);
    if (!list || !reference) { // NULL
        return -1;
    }  
    dplist_node_t* list_node = list->head;
    if (list_node == NULL) {
        return -1;
    }  
    int index = 0;
    while (list_node->next){
        if(list_node == reference){
            return index;
        }else {
            list_node = list_node->next;
        }
        index++;
    }     
    return index;
}

dplist_t *dpl_insert_at_reference(dplist_t *list, void *element, dplist_node_t *reference, bool insert_copy){ 
    dplist_node_t *list_node;
    if (list == NULL) return NULL;
    list_node = malloc(sizeof(dplist_node_t));
    printf("0\n");
    DPLIST_ERR_HANDLER(list_node == NULL, DPLIST_MEMORY_ERROR);
    if(insert_copy) list_node->element = list->element_copy_callback(element); 
    else list_node->element = element;
    dplist_node_t* ref = reference;
    if(!ref) {
        free(list_node);
        return NULL;
    }
    if (!list->head){
        list_node->prev = NULL;
        list_node->next = NULL;
        list->head = list_node;
        printf("1\n");
    } else if (dpl_get_index_of_reference(list, ref) <= 0){
        list_node->prev = NULL;
        list_node->next = list->head;
        list->head->prev = list_node;
        list->head = list_node;
        printf("2\n");
    } else{
        if(dpl_get_index_of_reference(list, ref) < dpl_size(list)){
            list_node->prev = ref->prev;
            list_node->next = ref;
            ref->prev->next = list_node;
            ref->prev = list_node;
            printf("3\n");     
        } else{ 
            assert(ref != NULL);
            list_node->next = NULL;
            list_node->prev = ref;
            ref->next = list_node;
            printf("4\n");
        }
    }
    return list;
}

dplist_t * dpl_insert_sorted(dplist_t * list, void * element, bool insert_copy)
{
    assert(list != NULL);
    if(insert_copy == true) element = (list->element_copy_callback)(element); // If element to copy
    dplist_node_t * new_node = (dplist_node_t *) malloc(sizeof(dplist_node_t));
    new_node->element = element;
    if(list->head == NULL) // If list is empty
    {
        list->head = new_node;
        new_node->prev = new_node->next = NULL;
    } else
    {
        dplist_node_t * dummy = list->head;
        while(dummy->next != NULL && (list->element_compare_callback)(new_node->element, dummy->element) > 0) // Goes through the collection until the end or until finds a member equal or larger than new_node
        {
            dummy = dummy->next;
        }
        if(dummy->next == NULL && (list->element_compare_callback)(new_node->element, dummy->element) >= 0) // If new_node larger than all members of the list or is equal to last, append it to the end of list
        {
            new_node->next = NULL;
            new_node->prev = dummy;
            dummy->next = new_node;
        } else if(dummy == list->head && (list->element_compare_callback)(new_node->element, dummy->element) < 0) // If new_node is the smallest in list
        {
            list->head = new_node;
            new_node->prev = NULL;
            new_node->next = dummy;
            dummy->prev = new_node;
        } else // If new_node is anywhere in the middle of list
        {
            new_node->next = dummy;
            new_node->prev = dummy->prev;
            dummy->prev->next = new_node;
            dummy->prev = new_node;
        }
    }
    return list;
}

void * dpl_get_element_at_reference( dplist_t * list, dplist_node_t * reference )
{
	DPLIST_ERR_HANDLER(list==NULL,DPLIST_INVALID_ERROR);
	if(list->head == NULL){ return NULL;}
	if(reference == NULL){
		return NULL;//dpl_get_element_at_index(list,dpl_size(list)+1); // returns the last element
	}
	for(int i=0; i<dpl_size(list)+1; i++){
		if(reference == dpl_get_reference_at_index(list,i)){
			return reference->element;
		}
	} return NULL;
}
