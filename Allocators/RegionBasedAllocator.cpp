/*
 * RegionBasedAllocator.cpp
 *
 *  Created on: 2015-11-03
 *      Author: GarCoSim
 *
 */

#include "RegionBasedAllocator.hpp"

extern int gLineInTrace;
using namespace std;


namespace traceFileSimulator {

RegionBasedAllocator::RegionBasedAllocator() {
}

bool RegionBasedAllocator::isRealAllocator() {
	return true;
}

bool RegionBasedAllocator::isInNewSpace(Object *object) {
	unsigned int heapIndex = getHeapIndex(object);
	return heapIndex >= newSpaceStartHeapIndex && heapIndex < newSpaceEndHeapIndex; 
}


void RegionBasedAllocator::moveObject(Object *object) {
	if (isInNewSpace(object))
		return;

	int size = object->getHeapSize();
	size_t address = (size_t)allocateInNewSpace(size);

	if (address == (size_t)-1) {
		fprintf(stderr, "error moving object (size %d) with id %d, old space %d, new space %d\n", size, object->getID(), getUsedSpace(false), getUsedSpace(true));
		exit(1);
	}
	memcpy((void *) address, (void *) object->getAddress(), size);

	object->updateAddress((void *) address);
	object->setForwarded(true);
}

void RegionBasedAllocator::initializeHeap(int heapSize) {
	overallHeapSize = heapSize;
	myHeapBitMap = new char[(int)ceil(heapSize/8.0) ];
	heap = (unsigned char*)malloc(heapSize); // * 8
	statLiveObjects = 0;
	resetRememberedAllocationSearchPoint();

	if (DEBUG_MODE && WRITE_ALLOCATION_INFO) {
		allocLog = fopen("alloc.log", "w+");
	}
	if (DEBUG_MODE && WRITE_HEAPMAP) {
		heapMap = fopen("heapmap.log", "w+");
	}
	fprintf(stderr, "heap size %zd\n", overallHeapSize);
}

void RegionBasedAllocator::freeAllSectors() {
	unsigned int i;
	for (i = 0; i < overallHeapSize; i++) {
		setBitUnused(i);
	}

}

void *RegionBasedAllocator::allocate(int size, int lower, int upper) {

	if (size <= 0)
		return NULL;
	
	unsigned int i, currentNumberEdenRegions, edenRegionID;
	int currentFreeSpace;
	void *currentFreeAddress;
	
	currentNumberEdenRegions = edenRegions.size();

	for (i = 0; i < currentNumberEdenRegions; i++) {
		edenRegionID = edenRegions.at(i);
		currentFreeSpace = balancedGCRegions[i]->getCurrFree();
		
		if (size <= currentFreeSpace) {
			currentFreeAddress = balancedGCRegions[edenRegionID]->getCurrFreeAddr();	
			balancedGCRegions[edenRegionID]->setCurrFreeAddr((void*)((long)currentFreeAddress+(long)size));
			balancedGCRegions[edenRegionID]->setCurrFree(currentFreeSpace-size);
			balancedGCRegions[edenRegionID]->incNumObj();
			
			setAllocated((long)currentFreeAddress, size);
			return &heap[(long)currentFreeAddress];
		}
	}
	
	if (currentNumberEdenRegions < maxNumberOfEdenRegions) {

		if (freeRegions.size() > 0) {
			edenRegionID = freeRegions.front();
			currentFreeSpace = balancedGCRegions[edenRegionID]->getCurrFree();
			
			if (size <= currentFreeSpace) {
				currentFreeAddress = balancedGCRegions[edenRegionID]->getCurrFreeAddr();
				balancedGCRegions[edenRegionID]->setCurrFreeAddr((void*)((long)currentFreeAddress+(long)size));
				balancedGCRegions[edenRegionID]->setCurrFree(currentFreeSpace-size);
				balancedGCRegions[edenRegionID]->incNumObj();
				
				freeRegions.erase(freeRegions.begin());
				edenRegions.push_back(edenRegionID);
				
				setAllocated((long)currentFreeAddress, size);
				return &heap[(long)currentFreeAddress];
			}
		}
	}

	return NULL;
}

unsigned int RegionBasedAllocator::getHeapIndex(Object *object) {
	// This conversion is only valid because the heap is an array of bytes.
	return (unsigned int) ((char *) object->getAddress() - (char *) heap);
}


void RegionBasedAllocator::gcFree(Object* object) {
	int size = object->getHeapSize();
	unsigned int heapIndex = getHeapIndex(object);

	setFree(heapIndex, size);
	statLiveObjects--;
}

RegionBasedAllocator::~RegionBasedAllocator() {
}

void RegionBasedAllocator::freeOldSpace() {
	setFree(oldSpaceStartHeapIndex, oldSpaceEndHeapIndex-oldSpaceStartHeapIndex);
}

}
