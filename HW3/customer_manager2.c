/* my name : 20150073 Kim Gyeongman
   assign # : assignment #3
   file name : customer_manager2.c */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "customer_manager.h"

#define UNIT_BUCKET_SIZE 1024

enum {HASH_MULTIPLIER = 65599}; //use in calculating hash
typedef struct Hashtable* HashTable_T;
/*UserInfo structer
  contain name, id pointer, purchase value, and struct UserInfo pointer
  this pointer point to next UserInfo in linked-list at same hash  */
struct UserInfo {
    char *name;                // customer name
    char *id;                  // customer id
    int purchase;              // purchase amount (> 0)
    struct UserInfo *next;
};
/*Data Base structer
  contain two HashTable_T(= struct Hashtable *) ht_~ , it's size and
  # of items. size is size of array which ht_~ point to.     */
struct DB {
    HashTable_T ht_id;         // pointer to the array
    HashTable_T ht_name;      // current array size (max # of elements)
    int numItems;             // # of stored items, needed to determine
    int curArrSize;           // # whether the array should be expanded
			     // # or not
};
/*Hashtable structer
  contain (struct UserInfo *)array[]. this array have pointer element
  point to UserInfo structer. Initial size is UNIT_BUCKET_SIZE(=1024)*/
struct Hashtable {
    struct UserInfo *array[UNIT_BUCKET_SIZE];
};

/*hash_fuction calculate hash value from char pointer parameter and
  iBucketCount parameter which means size of hashtable. This use
  HASH_MULTIPLIER and return hash value.           */
static int hash_function(const char *pcKey, int iBucketCount)
{
   int i;
   unsigned int uiHash = 0U;
   for (i = 0; pcKey[i] != '\0'; i++)         //calculate
      uiHash = uiHash * (unsigned int)HASH_MULTIPLIER
               + (unsigned int)pcKey[i];
   return (int)(uiHash % (unsigned int)iBucketCount);
}

/*--------------------------------------------------------------------*/
/*Create DB structer
  It allocate DB_T variable, it's ht_~ member and initialize numItems
  and curArrSize member. if there are any failure to allocate memory,
  return NULL. if they succeed, return start address of DB_T variable*/
DB_T                        //DB_T is type definition of (struct DB *)
CreateCustomerDB(void)
{
    DB_T d;

    d = (DB_T) calloc(1, sizeof(struct DB));   //allocate
    if (d == NULL) {
        fprintf(stderr, "Can't allocate a memory for DB_T\n");
        assert(0);
        return NULL;
    }
    d->curArrSize = UNIT_BUCKET_SIZE;   //initial size is 1024
    d->numItems = 0;
    d->ht_id = (struct Hashtable *)calloc(1, sizeof(struct Hashtable));
    if (d->ht_id == NULL) {
        fprintf(stderr,"Can't allocate memory for array of size %d\n",
            d->curArrSize);
        free(d);
        assert(0);
        return NULL;
    }
    d->ht_name=(struct Hashtable *)calloc(1, sizeof(struct Hashtable));
    if (d->ht_name == NULL) {
        fprintf(stderr,"Can't allocate memory for array of size %d\n",
            d->curArrSize);
        free(d->ht_id);
        free(d);
        assert(0);
        return NULL;
    }
    return d;
}
/*--------------------------------------------------------------------*/
/*Destroy DB_T variable
  free all allocated memory in ht_~ by using for() and ht_~.
  Finally, free DB_T variable. */
void
DestroyCustomerDB(DB_T d)
{
    /*same as assert()*/
    if (d == NULL || d->ht_name == NULL || d->ht_id == NULL) return ;

    struct UserInfo *p;
    struct UserInfo *nextp;
    int i;
    for (i=0; i<d->curArrSize; i++) {    //free element in hashtable
        for (p = d->ht_name->array[i]; p != NULL; p = nextp) {
            nextp = p->next;
            free(p->id);
            free(p->name);
            free(p);
        }
        for (p = d->ht_id->array[i]; p != NULL; p = nextp) {
            nextp = p->next;
            free(p->id);
            free(p->name);
            free(p);
        }
    }
    free(d->ht_name);               //free hashtable
    free(d->ht_id);
    free(d);                    //free DB_T variable
}
/*--------------------------------------------------------------------*/
/*Register new UserInfo to hashtable
  it take DB, id, name address and purchase value for parameter
  it search exist data for compare with new one for prevent duplicate.
  Then they allocate memory for make node and save information in
  that node. And add that node to proper hashtable by using
  hash_function. Finally, check their # of item exceed 75% of size.
  If exceed, extend ht_~ and recalculate hash value for all UserInfo in
  hashtable. Then add node for new hash value and delete original
  UserInfo data. It return 0 for succeed, and -1 for there are any
  failure.          */
int
RegisterCustomer(DB_T d, const char *id,
		 const char *name, const int purchase)
{
    if (d == NULL || id == NULL || name == NULL) return (-1);
    if (d->ht_name == NULL || d->ht_id == NULL) return (-1);
    if (purchase <= 0) return (-1);

    struct UserInfo *p;
    struct UserInfo *nextp;

    /*search for prevent duplicate*/
    int i_name = hash_function(name, d->curArrSize);
    for (p = d->ht_name->array[i_name]; p != NULL; p = nextp) {
        nextp = p->next;
        if (strcmp(p->name, name) == 0) return (-1);
    }

    int i_id = hash_function(id, d->curArrSize);
    for (p = d->ht_id->array[i_id]; p != NULL; p = nextp) {
        nextp = p->next;
        if (strcmp(p->id, id) == 0) return (-1);
    }

    /*allocate memory for save new data and add to name hashtable*/
    struct UserInfo *newp_name =
                    (struct UserInfo *)malloc(sizeof(struct UserInfo));
    if (newp_name == NULL) return (-1);
    newp_name->name = strdup(name);
    if (newp_name->name == NULL) {
        free(newp_name);
        return (-1);
    }
    newp_name->id = strdup(id);
    if (newp_name->id == NULL) {
        free(newp_name->name);
        free(newp_name);
        return (-1);
    }
    newp_name->purchase = purchase;
    newp_name->next = d->ht_name->array[i_name];    //add to hashtable
    d->ht_name->array[i_name] = newp_name;

    /*allocate memory for save new data and add to id hashtable*/
    struct UserInfo *newp_id =
                    (struct UserInfo *)malloc(sizeof(struct UserInfo));
    if (newp_id == NULL) return (-1);
    newp_id->name = strdup(name);
    if (newp_id->name == NULL) {
        free(newp_id);
        return (-1);
    }
    newp_id->id = strdup(id);
    if (newp_id->id == NULL) {
        free(newp_id->name);
        free(newp_id);
        return (-1);
    }
    newp_id->purchase = purchase;
    newp_id->next = d->ht_id->array[i_id];    //add to hashtable
    d->ht_id->array[i_id] = newp_id;

    (d -> numItems)++;

    /*If their need, extend size of hashtable*/
    if (d->numItems > (int)(0.75*d->curArrSize)
                                && d->curArrSize != 1000000) {
        int oldcurArrSize = d -> curArrSize;
        int newcurArrSize = d -> curArrSize * 2;
        if (newcurArrSize > 1000000) newcurArrSize = 1000000;

        int i;
        struct Hashtable *newht_id;
        struct Hashtable *newht_name;

        /*realloc() original hashtable*/
        newht_id = (struct Hashtable *)realloc(d->ht_id,
                        (newcurArrSize/1024)*sizeof(struct Hashtable));
        if (newht_id == NULL) {
            free(d);
            return (-1);
        }
        newht_name = (struct Hashtable *)realloc(d->ht_name,
                        (newcurArrSize/1024)*sizeof(struct Hashtable));
        if (newht_name == NULL) {
            free(d);
            free(newht_id);
            return (-1);
        }

        d->ht_id = newht_id;
        d->ht_name = newht_name;

        /*move all data to proper hash by using hash_fuction*/
        for (i = 0; i < oldcurArrSize; i++) {
            for (p = d->ht_id->array[i]; p != NULL; p = nextp) {
                nextp = p->next;

                i_name = hash_function(p->name, newcurArrSize);
                i_id = hash_function(p->id, newcurArrSize);
                /*allocate memory for add to id hashtable*/
                struct UserInfo *q_id =
                    (struct UserInfo *)malloc(sizeof(struct UserInfo));
                if (q_id == NULL) return (-1);
                q_id->purchase = p->purchase;
                q_id->id = strdup(p->id);
                if (q_id->id == NULL) {
                    free(q_id);
                    return (-1);
                }
                q_id->name = strdup(p->name);
                if (q_id->name == NULL) {
                    free(q_id->id);
                    free(q_id);
                    return (-1);
                }

                q_id->next = d->ht_id->array[i_id];  //add
                d->ht_id->array[i_id] = q_id;

                /*allocate memory for add to name hashtable*/
                struct UserInfo *q_name =
                    (struct UserInfo *)malloc(sizeof(struct UserInfo));
                if (q_name == NULL) return (-1);
                q_name->purchase = p->purchase;
                q_name->id = strdup(p->id);
                if (q_name->id == NULL) {
                    free(q_name);
                    return (-1);
                }
                q_name->name = strdup(p->name);
                if (q_name->name == NULL) {
                    free(q_name->id);
                    free(q_name);
                    return (-1);
                }

                q_name->next = d->ht_name->array[i_name]; //add
                d->ht_name->array[i_name] = q_name;
                (d -> numItems)++;

                /*delete original UserInfo data*/
                UnregisterCustomerByID(d, p->id);
            }
        }
        d->curArrSize = newcurArrSize; //assign new size
    }

   return 0;
}
/*--------------------------------------------------------------------*/
/*Unregister UserInfo data
  find data in proper hash by compare id parameter and data's id
  When find, save name value for unregist in ht_name. For Unregister,
  first liaise before node and next node. Then free that data memory.
  This return 0 for succeed and return -1 if there are any failure.  */
int
UnregisterCustomerByID(DB_T d, const char *id)
{
    if (d == NULL || id == NULL) return (-1);
    if (d->ht_name == NULL || d->ht_id == NULL) return (-1);

    struct UserInfo *p;
    struct UserInfo *nextp;
    struct UserInfo *beforep;
    char *name = NULL;
    int count = 0;

    int i_id = hash_function(id, d->curArrSize);
    beforep = d->ht_id->array[i_id];
    /*find in ht_id by using id parameter*/
    for (p = d->ht_id->array[i_id]; p != NULL; p = nextp) {
        nextp = p->next;
        if (strcmp(p->id, id) == 0) {
            name = strdup(p->name);
            if (name == NULL) return (-1);
            /*when there are just one element in hash*/
            if (p == d->ht_id->array[i_id] && nextp == NULL) {
                d->ht_id->array[i_id] = NULL;
                free(p->id);
                free(p->name);
                free(p);            //then free
                count++;          //count for check unregist in ht_id
                break;
            }
            /*when node we want to unregist is first, set
              next node to first element */
            else if (p == d->ht_id->array[i_id]) {
                d->ht_id->array[i_id] = nextp;
                free(p->id);
                free(p->name);
                free(p);           //then free
                count++;
                break;
            }
            /*when node we want to unregist is not first,
              just liaise before node and next */
            else {
                beforep -> next = nextp;
                free(p->id);
                free(p->name);
                free(p);             //then free
                count++;
                break;
            }
        }
        beforep = p;
    }

    if (name == NULL) return (-1);

    /*same as above but in ht_name is difference*/
    int i_name = hash_function(name, d->curArrSize);
    beforep = d->ht_name->array[i_name];
    for (p = d->ht_name->array[i_name]; p != NULL; p = nextp) {
        nextp = p->next;
        if (strcmp(p->name, name) == 0) {
            if (p == d->ht_name->array[i_name] && nextp == NULL) {
                d->ht_name->array[i_name] = NULL;
                free(p->id);
                free(p->name);
                free(p);
                count ++;
                break;
            }
            else if (p == d->ht_name->array[i_name]) {
                d->ht_name->array[i_name] = nextp;
                free(p->id);
                free(p->name);
                free(p);
                count++;
                break;
            }
            else {
                beforep -> next = nextp;
                free(p->id);
                free(p->name);
                free(p);
                count++;
                break;
            }
        }
        beforep = p;
    }
    free(name); //free for not memory leak

    if (count == 2) {  //count should 2 if succeed
        (d -> numItems)--;
        return 0;
    }

    return (-1);
}

/*--------------------------------------------------------------------*/
/*Unregister UserInfo data
  find data in proper hash by compare name parameter and data's name
  When find, save id value for unregist in ht_id. For Unregister,
  first liaise before node and next node. Then free that data memory.
  This return 0 for succeed and return -1 if there are any failure.  */
int
UnregisterCustomerByName(DB_T d, const char *name)
{
    if (d == NULL || name == NULL) return (-1);
    if (d->ht_name == NULL || d->ht_id == NULL) return (-1);

    struct UserInfo *p;
    struct UserInfo *nextp;
    struct UserInfo *beforep;
    char *id = NULL;
    int count = 0;

    int i_name = hash_function(name, d->curArrSize);
    beforep = d->ht_name->array[i_name];
     /*find in ht_name by using name parameter*/
    for (p = d->ht_name->array[i_name]; p != NULL; p = nextp) {
        nextp = p->next;
        if (strcmp(p->name, name) == 0) {
            id = strdup(p->id);
            if (id == NULL) return (-1);
            /*Three cases are same as above function*/
            if (p == d->ht_name->array[i_name] && nextp == NULL) {
                d->ht_name->array[i_name] = NULL;
                free(p->id);
                free(p->name);
                free(p);            //free
                count ++;
                break;
            }
            else if (p == d->ht_name->array[i_name]) {
                d->ht_name->array[i_name] = nextp;
                free(p->id);
                free(p->name);
                free(p);           //free
                count++;
                break;
            }
            else {
                beforep -> next = nextp;
                free(p->id);
                free(p->name);
                free(p);
                count++;             //free
                break;
            }
        }
        beforep = p;
    }

    if (id == NULL) return (-1);

    /*same as above but in ht_id is difference*/
    int i_id = hash_function(id, d->curArrSize);
    beforep = d->ht_id->array[i_id];
    for (p = d->ht_id->array[i_id]; p != NULL; p = nextp) {
        nextp = p->next;
        if (strcmp(p->id, id) == 0) {
            if (p == d->ht_id->array[i_id] && nextp == NULL) {
                d->ht_id->array[i_id] = NULL;
                free(p->id);
                free(p->name);
                free(p);
                count ++;
                break;
            }
            else if (p == d->ht_id->array[i_id]) {
                d->ht_id->array[i_id] = nextp;
                free(p->id);
                free(p->name);
                free(p);
                count++;
                break;
            }
            else {
                beforep -> next = nextp;
                free(p->id);
                free(p->name);
                free(p);
                count++;
                break;
            }
        }
        beforep = p;
    }
    free(id);      //free for nor memory leak

    if (count == 2) { //count should 2 if succeed
        (d -> numItems)--;
        return 0;
    }

    return (-1);
}
/*--------------------------------------------------------------------*/
/*Get purchase data from User who found by id parameter
  for all element in proper hash, compare their id with id parameter.
  If they find, assign User's purchase data to purchase variable.
  This return purchase value of founded User if succeed. If fail to
  search or there are any failure return -1.    */
int
GetPurchaseByID(DB_T d, const char* id)
{
    if (d == NULL || id == NULL) return (-1);
    if (d->ht_id == NULL) return (-1);

    struct UserInfo *p;
    struct UserInfo *nextp;
    int purchase;

    int i_id = hash_function(id, d->curArrSize);  //calculate hash
    for (p = d->ht_id->array[i_id]; p != NULL; p = nextp) {
        nextp = p->next;
        if (strcmp(p->id, id) == 0) {
            purchase = p->purchase;     //assign
            return purchase;
        }
    }

    return (-1);
}
/*--------------------------------------------------------------------*/
/*Get purchase data from User who found by name parameter
  for all element in proper hash, compare name data with name parameter.
  If they find, assign User's purchase data to purchase variable.
  This return purchase value of founded User if succeed. If fail to
  search or there are any failure return -1.    */
int
GetPurchaseByName(DB_T d, const char* name)
{
    if (d == NULL || name == NULL) return (-1);
    if (d->ht_name == NULL) return (-1);

    struct UserInfo *p;
    struct UserInfo *nextp;
    int purchase;

    int i_name = hash_function(name, d->curArrSize); //calculate hash
    for (p = d->ht_name->array[i_name]; p != NULL; p = nextp) {
        nextp = p->next;
        if (strcmp(p->name, name) == 0) {
            purchase = p->purchase;    //assign
            return purchase;
        }
    }

    return (-1);
}
/*--------------------------------------------------------------------*/
/*This function is repeat accumulate result of fp parameter
  fp parameter is function pointer and this need User's data parameter.
  If there are any failure, return -1. If not, return sum value which
  accumulate result of fp assign all element in hashtable    */
int
GetSumCustomerPurchase(DB_T d, FUNCPTR_T fp)
{
    if (d == NULL || fp == NULL) return (-1);
    if (d->ht_name == NULL) return (-1);

    int sum = 0, i;
    struct UserInfo *p;
    struct UserInfo *nextp;

    for (i = 0; i < d -> curArrSize; i++) {  //for all hash value
        for (p = d->ht_name->array[i]; p != NULL; p = nextp) {
            nextp = p->next;
            sum += fp(p->id, p->name, p->purchase);
        }                          //accumulate result of fp
    }

    return sum;
}
