/*
 * RegionBased.hpp
 *
 *  Created on: 2015-09-04
 *      Author: GarCoSim
 *
 * This collector implements a simple first-fit allocation policy.
 */

#include "RegionBased.hpp"

extern int gLineInTrace;
using namespace std;


namespace traceFileSimulator {

int edenLimit; //25% of heap size

RegionBased::RegionBased() {
}

bool RegionBased::isRealAllocator() {
	return true;
}

bool RegionBased::isInNewSpace(Object *object) {
	unsigned int heapIndex = getHeapIndex(object);
	return heapIndex >= newSpaceStartHeapIndex && heapIndex < newSpaceEndHeapIndex; 
}


void RegionBased::moveObject(Object *object) {
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

void RegionBased::initRegions(int heapSize) {
	int i;

    numRegions = (int)heapSize/REGIONSIZE;
    heapAddr   = (unsigned long)&heap[0];
    regions    = (Region**)malloc(sizeof(Region*)*numRegions);
    edenLimit  = (int)floor(numRegions*0.25);
    for (i=0; i<numRegions; i++) {
    	regions[i] = new Region((void*)(i*REGIONSIZE),REGIONSIZE,-1);
    	freeList.push_back(i);
    }
}


void RegionBased::initializeHeap(int heapSize) {
	overallHeapSize = heapSize;
	myHeapBitMap = new char[(int)ceil(heapSize/8.0) ];
	heap = (unsigned char*)malloc(heapSize * 8);
	statLiveObjects = 0;
	resetRememberedAllocationSearchPoint();

	if (DEBUG_MODE && WRITE_ALLOCATION_INFO) {
		allocLog = fopen("alloc.log", "w+");
	}
	if (DEBUG_MODE && WRITE_HEAPMAP) {
		heapMap = fopen("heapmap.log", "w+");
	}
	
    initRegions(heapSize);

    fprintf(stderr, "heap size %zd\n", overallHeapSize);
}

void RegionBased::freeAllSectors() {
	unsigned int i;
	for (i = 0; i < overallHeapSize; i++) {
		setBitUnused(i);
	}

}


void *RegionBased::allocate(int size, int lower, int upper) { //we keep this method for compatibility with other allocator.hpp
    if (size <= 0)
		return NULL;

	if ((int) oldSpaceRememberedHeapIndex < lower || (int) oldSpaceRememberedHeapIndex > upper)
		oldSpaceRememberedHeapIndex = lower; // essentially fall back to first fit

	int potentialStart, contiguous = 1;
	bool hasWrappedAround = false;
	for (potentialStart=oldSpaceRememberedHeapIndex+1; !hasWrappedAround || potentialStart<=(int)oldSpaceRememberedHeapIndex; potentialStart+=contiguous) {
		if (potentialStart > upper) {
			hasWrappedAround = true;
			potentialStart = lower;
			contiguous = 1;
		}

		if (isBitSet(potentialStart))
			continue;

		for (contiguous=1; contiguous<size; contiguous++) {
			if (potentialStart+contiguous > upper || isBitSet(potentialStart+contiguous))
				break;
		}
		if (contiguous == size) { // found a free slot big enough
			oldSpaceRememberedHeapIndex = potentialStart;
			setAllocated(potentialStart, size);
			return &heap[potentialStart];
		}
	}

	return NULL;
}

void *RegionBased::allocate(int size, int lower, int upper,int thread) {
	if (size <= 0) {
		return (void*)-1;
	}

	if (size > REGIONSIZE)
		return (void*)-2; 

    int i,rID,rOwner,currFree;
    void *currFreeAddr;
    
    i = 0;
    #ifdef THREADBASED
    while (i < numRegions) {
        rOwner = regions[i]->getOwner();
        if (rOwner == thread) 
        {	
        	currFree = regions[i]->getCurrFree();
            if (size <= currFree) {
               currFreeAddr = regions[i]->getCurrFreeAddr();	
               regions[i]->setCurrFreeAddr((void*)((long)currFreeAddr+(long)size));
               regions[i]->setCurrFree(currFree-size);
               regions[i]->incNumObj();
               setAllocated((long)currFreeAddr, size);
               return &heap[(long)currFreeAddr];
            }
        }
        i++;
    }
    if ((int)freeList.size()>0) {
    	rID   = freeList.front();
        currFree = regions[rID]->getCurrFree();
        currFreeAddr = regions[rID]->getAddress();	
        regions[rID]->setCurrFreeAddr((void*)((long)currFreeAddr+(long)size));
        regions[rID]->setCurrFree(currFree-size);
        regions[rID]->incNumObj();
        regions[rID]->setOwner(thread);
        freeList.erase(freeList.begin());
        setAllocated((long)currFreeAddr, size);
        return &heap[(long)currFreeAddr];
    }     
    #else
       i = 0;
       int numEden = (int)edenList.size();
       for (i=0; i<numEden; i++) {
       	    rID = edenList.at(i);
        	currFree = regions[rID]->getCurrFree();
            if (size <= currFree) {
               currFreeAddr = regions[rID]->getCurrFreeAddr();	
               regions[rID]->setCurrFreeAddr((void*)((long)currFreeAddr+(long)size));
               regions[rID]->setCurrFree(currFree-size);
               regions[rID]->incNumObj();
               setAllocated((long)currFreeAddr, size);
               return &heap[(long)currFreeAddr];
            }
        }
        if (numEden < edenLimit) {
    	    if ((int)freeList.size() > 0 ) {
    	        rID   = freeList.front();
                currFree = regions[rID]->getCurrFree();
                currFreeAddr = regions[rID]->getAddress();	
                regions[rID]->setCurrFreeAddr((void*)((long)currFreeAddr+(long)size));
                regions[rID]->setCurrFree(currFree-size);
                regions[rID]->incNumObj();

                freeList.erase(freeList.begin());
                edenList.push_back(rID);

                setAllocated((long)currFreeAddr, size);
                return &heap[(long)currFreeAddr];
            }
        }    
        else
            return (void*)-4; 
    #endif

	return (void*)-3; 
}

unsigned int RegionBased::getHeapIndex(Object *object) {
	// This conversion is only valid because the heap is an array of bytes.
	return (unsigned int) ((char *) object->getAddress() - (char *) heap);
}


void RegionBased::gcFree(Object* object) {
	int size = object->getHeapSize();
	unsigned int heapIndex = getHeapIndex(object);

	setFree(heapIndex, size);
	statLiveObjects--;
}

RegionBased::~RegionBased() {
}

void RegionBased::freeOldSpace() {
	setFree(oldSpaceStartHeapIndex, oldSpaceEndHeapIndex-oldSpaceStartHeapIndex);
}

}
