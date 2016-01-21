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
size_t          sumObj;
size_t          sumFree;

Region::Region(void *address,size_t size) {
	myAddress = address;
	mySize    = size;
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

void  Region::clearRemset() {
    myRemset.clear();
}

void Region::deleteFromRemset(void* obj) {
    myRemset.erase(obj);
}

Region::~Region() {
}

}
