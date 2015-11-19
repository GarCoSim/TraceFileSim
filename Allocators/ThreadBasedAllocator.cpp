/*
 * ThreadBasedAllocator.hpp
 *
 *  Created on: 2015-09-04
 *      Author: GarCoSim
 *
 */

#include "ThreadBasedAllocator.hpp"

extern int gLineInTrace;
using namespace std;


namespace traceFileSimulator {

size_t edenLimit; //25% of heap size

ThreadBasedAllocator::ThreadBasedAllocator() {
}

bool ThreadBasedAllocator::isRealAllocator() {
	return true;
}

bool ThreadBasedAllocator::isInNewSpace(Object *object) {
	size_t heapIndex = getHeapIndex(object);
	return heapIndex >= newSpaceStartHeapIndex && heapIndex < newSpaceEndHeapIndex; 
}


void ThreadBasedAllocator::moveObject(Object *object) {
	if (isInNewSpace(object))
		return;

	size_t size = object->getHeapSize();
	size_t address = (size_t)allocateInNewSpace(size);

	if (address == (size_t)-1) {
		fprintf(stderr, "error moving object (size %zu) with id %d, old space %zu, new space %zu\n", size, object->getID(), getUsedSpace(false), getUsedSpace(true));
		exit(1);
	}
	memcpy((void *) address, (void *) object->getAddress(), size);

	object->updateAddress((void *) address);
	object->setForwarded(true);
}

void ThreadBasedAllocator::initRegions(size_t heapSize) {
	int i;

    numRegions = (int)heapSize/REGIONSIZE;
    heapAddr   = (size_t)&heap[0];
    regions    = (ThreadOwnedRegion**)malloc(sizeof(ThreadOwnedRegion*)*numRegions);
    edenLimit  = (size_t)floor(numRegions*0.25);
    for (i=0; i<numRegions; i++) {
    	 regions[i] = new ThreadOwnedRegion((void*)(i*REGIONSIZE),REGIONSIZE,-1);
    	 freeList.push_back(i);
    }
}


void ThreadBasedAllocator::initializeHeap(size_t heapSize) {
  	overallHeapSize = heapSize;
	  myHeapBitMap = new char[(size_t)ceil(heapSize/8.0) ];
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

void ThreadBasedAllocator::freeAllSectors() {
	size_t i;
	for (i = 0; i < overallHeapSize; i++) {
		setBitUnused(i);
	}

}


void *ThreadBasedAllocator::allocate(size_t size, size_t lower, size_t upper) { //we keep this method for compatibility with other allocator.hpp
    if (size <= 0)
		return NULL;

	if ((size_t) oldSpaceRememberedHeapIndex < lower || (size_t) oldSpaceRememberedHeapIndex > upper)
		oldSpaceRememberedHeapIndex = lower; // essentially fall back to first fit

	size_t potentialStart, contiguous = 1;
	bool hasWrappedAround = false;
	for (potentialStart=oldSpaceRememberedHeapIndex+1; !hasWrappedAround || potentialStart<=(size_t)oldSpaceRememberedHeapIndex; potentialStart+=contiguous) {
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

void *ThreadBasedAllocator::allocate(size_t size, size_t lower, size_t upper,int thread) {
	if (size <= 0) {
		return (void*)-1;
	}

	if (size > REGIONSIZE)
	 	 return (void*)-2; 

  int i, rID, rOwner;
  size_t currFree;
  void *currFreeAddr;
    
  i = 0;

  while (i < numRegions) {
      rOwner = regions[i]->getOwner();
      if (rOwner == thread) {	
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

	return (void*)-3; 
}

size_t ThreadBasedAllocator::getHeapIndex(Object *object) {
	// This conversion is only valid because the heap is an array of bytes.
	return (size_t) ((char *) object->getAddress() - (char *) heap);
}


void ThreadBasedAllocator::gcFree(Object* object) {
	size_t size = object->getHeapSize();
	size_t heapIndex = getHeapIndex(object);

	setFree(heapIndex, size);
	statLiveObjects--;
}

ThreadBasedAllocator::~ThreadBasedAllocator() {
}

void ThreadBasedAllocator::freeOldSpace() {
	setFree(oldSpaceStartHeapIndex, oldSpaceEndHeapIndex-oldSpaceStartHeapIndex);
}

}
