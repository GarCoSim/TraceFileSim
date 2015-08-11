/*
 * RealAllocator.cpp
 *
 *  Created on: 2015-03-04
 *      Author: GarCoSim
 */

#include "RealAllocator.hpp"

extern int gLineInTrace;
extern double totTime;

using namespace std;

namespace traceFileSimulator {

RealAllocator::RealAllocator() {
}

bool RealAllocator::isRealAllocator() {
	return true;
}

bool RealAllocator::isInNewSpace(Object *object) {
	int address = getLogicalAddress(object);
	
	if (address >= newSpaceOffset && address < myHeapSizeNewSpace)
		return true;

	return false;
}


void RealAllocator::moveObject(Object *object) {

	if (isInNewSpace(object))
		return;

	Object *temp;

	int size = object->getPayloadSize();

	size_t address = (size_t)allocateInNewSpace(size);
	if (address == (size_t)-1) {
		fprintf(stderr, "error moving object (size %d), old space %d, new space %d\n", size, getUsedSpace(false), getUsedSpace(true));
		exit(1);
	}

	temp = (Object*)address;

    //memcpy(&temp,&object,sizeof(Object*)); // this doesn't work, need to fix later, can't figure out why right now]	
	// we do a hack for now
	//temp->setArgs(object->getID(), object->getPayloadSize(), object->getPointersMax(), (char*)object->getClassName());
	//temp->resetPtrs(object->getPointersMax());   
	//temp->setVisited(1);
    
	temp->setArgsReal(object->getID(), object->getPayloadSize(), object->getPointersMax(), (char*)object->getClassName());

	object->updateAddress(address);
	object->setForwarded(true);

	object = temp; 
}

void RealAllocator::initializeHeap(int heapSize) {
	myHeapBitMap = new char[heapSize / 8 + 1];

	myHeapSizeOldSpace = heapSize;
	myLastSuccessAddressOldSpace = 0;
	myLastSuccessAddressNewSpace = heapSize / 2;
	myHeapSizeNewSpace = heapSize;

	statBytesAllocated = 0;
	statLiveObjects = 0;
	if (DEBUG_MODE && WRITE_ALLOCATION_INFO) {
		allocLog = fopen("alloc.log", "w+");
	}
	if (DEBUG_MODE && WRITE_HEAPMAP) {
		heapMap = fopen("heapmap.log", "w+");
	}
	newSpaceOffset = heapSize / 2;
	oldSpaceOffset = 0;
	overallHeapSize = heapSize;
	fprintf(stderr, "heap size %d\n", overallHeapSize);

	heap = (unsigned char*)malloc(heapSize * 8);
	myLastSuccessAddressOldSpace = (size_t)&heap[0];
	myLastSuccessAddressNewSpace = (size_t)&heap[0];
}

void RealAllocator::freeAllSectors() {
	int i;

	for (i = 0; i < overallHeapSize; i++) {
		setBitUnused(i);
	}

	statBytesAllocated = 0;
}

size_t RealAllocator::allocate(int size, int lower, int upper, size_t lastAddress) {
	//added by Tristan
    struct timeval tv;
	unsigned long time_start;

	if (size <= 0) {
		return -1;
	}

	int address = lower;
	int contiguous = 0;
	int i, bit;
	int passedBoundOnce = 0;

    //added by Tristan
    //gettimeofday(&tv, NULL); 
    //time_start = 1000000*tv.tv_sec+tv.tv_usec;

	for (i = lower; i <= upper; i++) {
/*
    	if (i == upper) {
			if (passedBoundOnce == 1) {
				return -1;
			}
			i = lower;
			contiguous = 0;
			address = i + 1;
			passedBoundOnce = 1;
		}
*/		
// **************** Added by Tristan *****************
	/*	
        if (i%32==0) {
           long v = (long)myHeapBitMap[i/8];
           if (v == -1) {
			  i += 32;
			  continue;
		   }
		}
	

        if (i%16==0) {
           short v = (short)myHeapBitMap[i/8];
           if (v == -1) {
			  i += 16;
			  continue;
		   }
		}
	*/	

    	if (i%8==0 && myHeapBitMap[i/8] == (char)255) {
			i += 8;
			continue;
		}

		
// *************** End of Add ************************


        bit = isBitSet(i);

		if (bit == 1) {
			address = i + 1;
			contiguous = 0;
		} else {
			contiguous++;
			if (contiguous == size) {
				setAllocated(address, size);
				statBytesAllocated += size;
				statLiveObjects++;
				return (size_t)&heap[address];
			}
		}
	}
	//added by Tristan
	//gettimeofday(&tv, NULL);
	//totTime = ((1000000*tv.tv_sec+tv.tv_usec) - time_start)/1.0e6;

	return -1;
}

size_t RealAllocator::getLogicalAddress(Object *object) {
	size_t address = (size_t)object;
	size_t i;

	// find the address we're looking for
	for (i = 0; i < (size_t)overallHeapSize; i++)
		if (address == (size_t)&heap[i])
			return i;

	return -1;
}

void RealAllocator::gcFree(Object* object) {
	int size = object->getPayloadSize();

	setFree(getLogicalAddress(object), size);

	statLiveObjects--;
	statBytesAllocated -= size;
}

RealAllocator::~RealAllocator() {
}

}
