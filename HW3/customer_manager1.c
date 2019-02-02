/* my name : 20150073 Kim Gyeongman
   assign # : assignment #3
   file name : customer_manager1.c */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "customer_manager.h"

#define UNIT_ARRAY_SIZE 1024
/*UserInfo structer
  contain name, id pointer and purchase value   */
struct UserInfo {
    char *name;                // customer name
    char *id;                  // customer id
    int purchase;              // purchase amount (> 0)
};

/*Data Base structer
  contain UserInfo array and it's size, # of items, and maxnum value
  maxnum value is biggest index which contain valid element*/
struct DB {
    struct UserInfo *pArray;   // pointer to the array
    int curArrSize;           // current array size (max # of elements)
    int numItems;             // # of stored items, needed to determine
	int maxnum;		     // # whether the array should be expanded
			     // # or not
};

/*--------------------------------------------------------------------*/
/*Create DB structer
  It allocate DB_T variable and it's parray member
  if they fail to allocate memory, return NULL
  if they succeed, return start address of DB_T variable  */
DB_T             //DB_T is type definition of (struct DB *)
CreateCustomerDB(void)
{
    DB_T d;

    d = (DB_T) calloc(1, sizeof(struct DB));
    if (d == NULL) {
        fprintf(stderr, "Can't allocate a memory for DB_T\n");
        assert(0);
        return NULL;
    }
    d->curArrSize = UNIT_ARRAY_SIZE; // start with 1024 elements
    d->numItems = 0;            //initiate # of item and maxnum  0
    d->maxnum = 0;
    d->pArray = (struct UserInfo *)calloc(d->curArrSize,
                sizeof(struct UserInfo));
    if (d->pArray == NULL) {
        fprintf(stderr,"Can't allocate memory for array of size %d\n",
            d->curArrSize);
        free(d);              //should free memory for not memory leak
        assert(0);
        return NULL;
    }
    return d;
}
/*--------------------------------------------------------------------*/
/*Destroy DB_T variable
  free all allocated memory in it and it too */
void
DestroyCustomerDB(DB_T d)
{
    if (d == NULL) return ;       //should do nothing

    free(d -> pArray);
    free(d);
}
/*--------------------------------------------------------------------*/
/*Register new UserInfo to array
  it take DB, id, name address and purchase value for parameter
  it search exist data for compare with new one for prevent duplicate
  and then check there are memory for save this data. If not, resize
  array size. Finally, it save UserInfo to array. It return 0 for
  succeed, and -1 for there are any failure.          */
int
RegisterCustomer(DB_T d, const char *id,
		 const char *name, const int purchase)
{
    if (d == NULL || id == NULL || name == NULL) return (-1);
    if (purchase <= 0) return (-1);

    int i;
    for (i = 0; i < d -> maxnum; i++) {           //prevent duplicate
        if ((d -> pArray)[i].name != NULL &&
                  strcmp((d -> pArray)[i].name, name) == 0) return(-1);
        if ((d -> pArray)[i].name != NULL &&
                  strcmp((d -> pArray)[i].id, id) == 0) return (-1);
    }

    if (d -> numItems == d -> curArrSize) {    //extend array size
        struct UserInfo *newpArray;
        int newcurArrSize = d -> curArrSize + UNIT_ARRAY_SIZE;
        newpArray = (struct UserInfo *)realloc(d -> pArray, //realloc
                            newcurArrSize * sizeof(struct UserInfo));
        if (newpArray == NULL) return (-1);
        d -> curArrSize = newcurArrSize;
        d -> pArray = newpArray;
    }

    for (i = 0; i <= d -> maxnum; i++) {
        if ((d -> pArray)[i].name == NULL) {    //if there are empty
            (d -> pArray)[i].name = strdup(name);  //save data
            if ((d -> pArray)[i].name == NULL) return (-1);
            (d -> pArray)[i].id = strdup(id);  //use strdup for own val
            if ((d -> pArray)[i].id == NULL) { //If fail to allocate
                free((d -> pArray)[i].name);
                return (-1);
            }
            (d -> pArray)[i].purchase = purchase;
            (d -> numItems)++;
            if (i == d -> maxnum) {//If new data's array index is last,
                (d -> maxnum)++;    //increase so maxnum is always
            }                       //last element  index + 1
            return 0;
        }
    }

    return (-1);
}
/*--------------------------------------------------------------------*/
/*Unregister UserInfo data
  find data index by compare id parameter and array data's id
  we should check there is valid element before use strcmp function
  because we put NULL in strcmp, this make error. For Unregister, first
  free that data memory and change name value to NULL for meaning empty.
  This return 0 for succeed and return -1 if there are any failure.  */
int
UnregisterCustomerByID(DB_T d, const char *id)
{
    if (d == NULL || id == NULL) return (-1);

    int i;
    for (i = 0; i < d -> maxnum; i++) {
        if ((d -> pArray)[i].name != NULL &&  //check valid element
                        strcmp((d -> pArray)[i].id, id) == 0) {
            free((d -> pArray)[i].id);
            free((d -> pArray)[i].name);
            (d -> pArray)[i].name = NULL;    //mean empty element
            (d -> numItems)--;
            if (i == d -> maxnum - 1) {
                (d -> maxnum)--;    //if unreg last element, decrease
            }                       //maxnum because it mean last
            return 0;               //valid element index + 1
        }
    }

    return (-1);
}

/*--------------------------------------------------------------------*/
/*Unregister UserInfo data
  find data index by compare name parameter and array data's name
  we should check there is valid element before use strcmp function
  because we put NULL in strcmp, this make error. For Unregister, first
  free that data memory and change name value to NULL for meaning empty.
  This return 0 for succeed and return -1 if there are any failure.  */
int
UnregisterCustomerByName(DB_T d, const char *name)
{
    if (d == NULL || name == NULL) return (-1);

    int i;
    for (i = 0; i < d -> maxnum; i++) {
        if ((d -> pArray)[i].name != NULL &&  //check valid element
                        strcmp((d -> pArray)[i].name, name) == 0) {
            free((d -> pArray)[i].id);
            free((d -> pArray)[i].name);
            (d -> pArray)[i].name = NULL; //mean empty element
            (d -> numItems)--;
            if (i == d -> maxnum - 1) {
                (d -> maxnum)--;      //same as above
            }
            return 0;
        }
    }

    return (-1);
}
/*--------------------------------------------------------------------*/
/*Get purchase data from User who found by id parameter
  for all element, compare their id with id parameter for find User.
  If they find, assign User's purchase data to purch variable.
  This return purchase value of founded User if succeed. If fail to
  search or there are any failure return -1.    */
int
GetPurchaseByID(DB_T d, const char* id)
{
    if (d == NULL || id == NULL) return (-1);

    int i, purch;
    for (i = 0; i < d -> maxnum; i++) {
        if ((d -> pArray)[i].name != NULL &&  //check valid element
                        strcmp((d -> pArray)[i].id, id) == 0) {
            purch = (d -> pArray)[i].purchase;  //assign
            return purch;
        }
    }

    return (-1);
}
/*--------------------------------------------------------------------*/
/*Get purchase data from User who found by name parameter
  for all element, compare their name with name parameter for find User.
  If they find, assign User's purchase data to purch variable.
  This return purchase value of founded User if succeed. If fail to
  search or there are any failure return -1.    */
int
GetPurchaseByName(DB_T d, const char* name)
{
    if (d == NULL || name == NULL) return (-1);

    int i, purch;
    for (i = 0; i < d -> maxnum; i++) {
        if ((d -> pArray)[i].name != NULL &&   //check valid element
                        strcmp((d -> pArray)[i].name, name) == 0) {
            purch = (d -> pArray)[i].purchase;  //assign
            return purch;
        }
    }

    return (-1);
}
/*--------------------------------------------------------------------*/
/*This function is repeat accumulate result of fp parameter
  fp parameter is function pointer and this need User's data parameter.
  If there are any failure, return -1. If not, return sum value which
  accumulate result of fp assign all element in array.    */
int
GetSumCustomerPurchase(DB_T d, FUNCPTR_T fp)
{
    if (d == NULL || fp == NULL) return (-1);
    int sum = 0, i;
    for (i = 0; i < d -> maxnum; i++) {
        if ((d -> pArray)[i].name != NULL) {   //check valid element
            sum += fp((d -> pArray)[i].id, (d -> pArray)[i].name,
                                            (d -> pArray)[i].purchase);
        }                //accumulate result of fp by using for()
    }
    return sum;
}
