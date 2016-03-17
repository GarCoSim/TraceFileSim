/*
 * Region.hpp
 *
 *  Created on: September 1, 2015
 *      Author: Tristan
 */

#ifndef REGION_HPP_
#define REGION_HPP_

#include <vector>
#include <set>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <math.h>
#include "Object.hpp"

#define REGIONSIZE 4194304//524288 //From Tristan

using std::vector;
namespace traceFileSimulator {

class Region {
public:
	virtual ~Region();	
	Region(void *address, size_t size, size_t heapAddress);
	
    void setSize(size_t size);
    size_t  getSize();

    void  setAddress(void* address);
    void* getAddress();
         
    void setCurrFree(size_t free); 
    size_t  getCurrFree(); 

    void incrementObjectCount(); 
    int  getNumObj(); 

    void  setCurrFreeAddr(void* addr); 
    void* getCurrFreeAddr(); 

    void  appendObj(long obj); 
    char* getObjects(); 

    void  setAge(int age);
    void  incAge();
    int   getAge();

    std::set<void*> getRemset();

    void  insertObjectReference(void* address);
    void  eraseObjectReference(void* address);
    void  eraseObjectReferenceWithoutCheck(void* address);

    void reset();

protected:
	size_t   mySize;
	size_t myHeapAddress;
	void* myAddress;
    int   myAge;
    std::set<void*> myRemset; //remembered set 

    int   numObj;        //how many objects in the region
	size_t   currFree;      //how much free space in a region
	void  *currFreeAddr; //address of the free area in the region
};

//From Tristan
extern int numRegions;
extern size_t heapAddr;

extern std::vector<int> freeList;
extern std::vector<int> edenList;

extern size_t sumObj;
extern size_t sumFree;
extern int  trigReason;

extern Region** regions;

}
#endif /* REGION_HPP_ */
