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

#define REGIONSIZE 4194304//524288

using std::vector;
namespace traceFileSimulator {

class Region {
public:
	virtual ~Region();	
	Region(void *address, int size);
	
    void setSize(int size);
    int  getSize();

    void  setAddress(void* address);
    void* getAddress();
         
    void setCurrFree(int free); 
    int  getCurrFree(); 

    void incNumObj(); 
    int  getNumObj(); 

    void  setCurrFreeAddr(void* addr); 
    void* getCurrFreeAddr(); 

    void  appendObj(long obj); 
    char* getObjects(); 

    int   getRegion(long address);

    void  setAge(int age);
    void  incAge();
    int   getAge();

    void  insertRemset(void *obj);
    void  eraseRemset(void *obj);

protected:
	int   mySize;
	void* myAddress;
    int   myAge;
    std::set<void*> myRemset; //remembered set 

    int   numObj;        //how many objects in the region
	int   currFree;      //how much free space in a region
	void  *currFreeAddr; //address of the free area in the region
};

extern int           numRegions;
extern unsigned long heapAddr;

extern std::vector<int> freeList;
extern std::vector<int> edenList;

extern long sumObj;
extern long sumFree;
extern int  trigReason;

} 
#endif /* REGION_HPP_ */
