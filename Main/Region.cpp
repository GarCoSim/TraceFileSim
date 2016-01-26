/*
 * Region.cpp
 *
 *  Created on: Jul 30, 2013
 *      Author: GarCoSim
 */

#include "../defines.hpp"
#include "Region.hpp"

extern int gLineInTrace; //added by Tristan

namespace traceFileSimulator {


Region::Region(void *address,size_t size) {
	myAddress = address;
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

void Region::insertObjectReference(void* obj) {
    myRemset.push_back(obj);
	fprintf(stderr, "insertObjectReference: %p\n", obj);
}

void Region::eraseObjectReference(void* obj) {
    std::vector<void*>::iterator iterator;

	for(iterator = myRemset.begin();  iterator != myRemset.end(); iterator++)
	{
		fprintf(stderr, "eraseObjectReference: %p\n", *iterator);
	}
	
    //it = myRemset.find(obj);
    //if (it != myRemset.end() )
	 //  myRemset.erase(myRemset.find(obj));
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
