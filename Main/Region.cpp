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

Region**      regions;
int           numRegions = 0;
unsigned long heapAddr;

std::vector<int>   freeList;
std::vector<int>   edenList;

int           trigReason = 0;
long          sumObj;
long          sumFree;

Region::Region(void *address,int size,int owner) {
	myAddress = address;
	mySize    = size;
	myOwner   = owner;
	numObj       = 0;
	currFreeAddr = address;
	currFree     = size;
}

void Region::setAddress(void* address) {
    myAddress = address;
}

void* Region::getAddress() {
	return myAddress;
}

void Region::setSize(int size) {
   mySize = size;
}

int Region::getSize() {
	return mySize;
}

void Region::setOwner(int owner) {
	myOwner = owner;
}

int Region::getOwner() {
	return myOwner;
}

void Region::setCurrFree(int free) {
    currFree = free;
} 

int  Region::getCurrFree() {
    return currFree;
}

void Region::incNumObj() {
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

void Region::insertRemset(void* obj) {
    myRemset.insert(obj);
}

void Region::eraseRemset(void* obj) {
    std::set<void*>::iterator it;

    it = myRemset.find(obj);
    if (it != myRemset.end() )
	   myRemset.erase(myRemset.find(obj));
}

Region::~Region() {
}

}
