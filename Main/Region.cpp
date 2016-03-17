/*
 * Region.cpp
 *
 *  Created on: Jul 30, 2013
 *      Author: GarCoSim
 */

#include "../defines.hpp"
#include "Region.hpp"

extern int gLineInTrace; //added by Tristan
extern FILE* balancedLogFile;

namespace traceFileSimulator {


Region::Region(void *address,size_t size, size_t heapAddress) {
	myAddress = address;
	myHeapAddress = heapAddress;
	mySize    = size;
	numObj       = 0;
	currFreeAddr = address;
	currFree     = size;
	myAge = 0;
}

void Region::setAddress(void* address) {
    myAddress = address;
}

void* Region::getAddress() {
	return myAddress;
}

void Region::setSize(size_t size) {
   mySize = size;
}

size_t Region::getSize() {
	return mySize;
}

void Region::setCurrFree(size_t free) {
    currFree = free;
} 

size_t  Region::getCurrFree() {
    return currFree;
}

void Region::incrementObjectCount() {
    numObj++;
}

int Region::getNumObj() {
    return numObj;
}

void Region::setCurrFreeAddr(void* addr) {
    currFreeAddr = addr;
} 

void* Region::getCurrFreeAddr() {
    return currFreeAddr;
}

void Region::setAge(int age) {
    myAge = age;
} 

void Region::incAge() {
    myAge++;
} 

int Region::getAge() {
    return myAge;
}

std::set<void*> Region::getRemset() {
    return myRemset;
}

void Region::reset() {
    numObj       = 0;
    currFreeAddr = myAddress;
    currFree     = mySize;
    myAge = 0;
    myRemset.clear();
}

void Region::insertObjectReference(void* address) {
        myRemset.insert(address);
}


void Region::eraseObjectReferenceWithoutCheck(void *address) {
    myRemset.erase(myRemset.find(address));
}

void Region::eraseObjectReference(void *address) {	
	std::set<void*>::iterator it;
	
    it = myRemset.find(address);
    if (it != myRemset.end()) {
        Object *child;
        int k;
        int thisRegion = (unsigned long)myAddress/mySize;
        int childRegion;

        RawObject *rawObject = (RawObject *) address;
        Object *obj = rawObject->associatedObject;

        int numChild   = obj->getPointersMax(); //check if no more child is pointing to this region
        for (k=0; k<numChild; k++) {
            child = obj->getReferenceTo(k);
            if (child) {
				childRegion = (unsigned int)(((size_t)child->getAddress()-myHeapAddress)/mySize); //need this!
				
                if (thisRegion == childRegion) //if a child points to this region, then don't erase from remset
                    return; 
            }
        }
        myRemset.erase(myRemset.find(address));
    }
	
}

Region::~Region() {
}


//From Tristan
Region**      regions;
int           numRegions = 0;
unsigned long heapAddr;

std::vector<int>   freeList;
std::vector<int>   edenList;

int           trigReason = 0;
size_t          sumObj;
size_t          sumFree;
}
