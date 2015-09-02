/*
 * RealAllocator.cpp
 *
 *  Created on: 2015-03-04
 *      Author: GarCoSim
 */

#include "RealAllocator.hpp"

extern int gLineInTrace;

extern double totTime; //added by Tristan


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

	int size = object->getHeapSize();
	size_t address = (size_t)allocateInNewSpace(size);

	if (address == (size_t)-1) {
		fprintf(stderr, "error moving object (size %d) with id %d, old space %d, new space %d\n", size, object->getID(), getUsedSpace(false), getUsedSpace(true));
		exit(1);
	}
	memcpy((void *) address, (void *) object->getAddress(), size);

	object->updateAddress((size_t) address);
	object->setForwarded(true);
}

void RealAllocator::initializeHeap(int heapSize) {
	myHeapBitMap = new char[(int)ceil(heapSize/8.0) ]; //modified by Tristan to use ceil() function

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

	card1 = new CardTable(8,(long)heapSize);  //cardTable to represent 8-bits of the bitmap
	card2 = new CardTable(64,(long)heapSize); //fixing some issues
}

void RealAllocator::freeAllSectors() {
	int i;

	for (i = 0; i < overallHeapSize; i++) {
		setBitUnused(i);
	}

	statBytesAllocated = 0;
}

size_t RealAllocator::allocate(int size, int lower, int upper, size_t lastAddress) {
	if (size <= 0) {
		return -1;
	}

	int address = lower;
	int contiguous = 0;
	int i,j,k,bit,idx;

    i = lower;
    while (i < upper) {
        if (card2->isCardMarked((long)i)) {
        	i = address = card2->nextCardAddress(i);
        	contiguous = 0;
        	continue;
        }
        j = i;
        while (j<upper && j<i+64) {
	        if (card1->isCardMarked(j)) {
	    	   j = address = card1->nextCardAddress(j);
	      	   contiguous = 0;
               continue;
            }
            k = j;
            while (k<upper && k<j+8) {
		        bit = isBitSet(k);
		        if (bit == 1) {
			        address = k+1;
			        contiguous = 0;
		        } else {
			        contiguous++;
			        if (contiguous == size) {
			            setAllocated(address, size);
				        statBytesAllocated += size;
				        statLiveObjects++;

                        //added by Tristan
                        card1->markCards8(address,size,myHeapBitMap);
                        card2->markCards64(address,size,myHeapBitMap); 
                       
				        return (size_t)&heap[address];
				    }
			    }
			    k++;
		    }
		    j+=8;
		}
		i+=64;
	}
	return -1;
}

inline size_t RealAllocator::getLogicalAddress(Object *object) {
	return (size_t) object->getAddress() - (size_t) heap;
}

void RealAllocator::gcFree(Object* object) {
	int size = object->getHeapSize();
	int address = getLogicalAddress(object);

	setFree(address, size);

    card1->unmarkCards(address,size,myHeapBitMap); //added by Tristan
    card2->unmarkCards(address,size,myHeapBitMap); //added by Tristan

	statLiveObjects--;
	statBytesAllocated -= size;
}

RealAllocator::~RealAllocator() {
}

void RealAllocator::freeOldSpace() {
	setFree(oldSpaceOffset, myHeapSizeOldSpace-oldSpaceOffset);
	card1->syncCards8(myHeapBitMap);
	card2->syncCards64(myHeapBitMap);
}

}
