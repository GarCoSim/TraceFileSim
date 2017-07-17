#include "Region.hpp"

extern LINESIZE gLineInTrace;
extern FILE* balancedLogFile;

namespace traceFileSimulator {

/** Constructor for a Region object.
 *
 * @param address Internal address pointer for space in the region.
 * @param size Size of a region.
 * @param heapAddress Pointer to the address of the region in the heap.
 */
Region::Region(void *address,size_t size, unsigned char* heapAddress) {
	myAddress = address;
	myHeapAddress = heapAddress;
	mySize = size;
	numObj = 0;
	currFreeAddr = address;
	currFree = size;
	myAge = 0;
	isArrayletleaf = false;
	associatedSpine = NULL;
}

void* Region::getAddress() {
	return myAddress;
}

unsigned char* Region::getHeapAddress(){
	return myHeapAddress;
}

void Region::setSize(size_t size) {
	mySize = size;
}

void Region::setCurrFree(size_t free) {
	currFree = free;
} 

size_t Region::getCurrFree() {
	return currFree;
}

void Region::incrementObjectCount() {
	numObj++;
}

int Region::getNumObj() {
	return numObj;
}

void Region::setNumObj(int objects){
	numObj = objects;
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

int Region::getAge() {
	return myAge;
}

std::set<void*> Region::getRemset() {
	return myRemset;
}

void Region::setIsLeaf(bool isLeaf) {
	isArrayletLeaf = isLeaf;
}

bool Region::getIsLeaf() {
	return isArrayletLeaf;
}

/** Resets frequently changing variables to their defaults.
 *
 */
void Region::reset() {
	numObj = 0;
	currFreeAddr = myAddress;
	currFree = mySize;
	myAge = 0;
	myRemset.clear();
	spineRemset.clear();
	isArrayletleaf = false;
}

void Region::insertObjectReference(void* address) {
	myRemset.insert(address);
}


void Region::eraseObjectReferenceWithoutCheck(void *address) {
	myRemset.erase(myRemset.find(address));
}

Region::~Region() {
}

}
