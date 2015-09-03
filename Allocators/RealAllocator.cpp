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
	unsigned int heapIndex = getHeapIndex(object);
	return heapIndex >= newSpaceStartHeapIndex && heapIndex < newSpaceEndHeapIndex; 
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

	object->updateAddress((void *) address);
	object->setForwarded(true);
}

void RealAllocator::initializeHeap(int heapSize) {
	myHeapBitMap = new char[(int)ceil(heapSize/8.0) ]; //modified by Tristan to use ceil() function

	overallHeapSize = heapSize;
	setHalfHeapSize(heapSize);
	resetRememberedAllocationSearchPoint();

	statLiveObjects = 0;
	if (DEBUG_MODE && WRITE_ALLOCATION_INFO) {
		allocLog = fopen("alloc.log", "w+");
	}
	if (DEBUG_MODE && WRITE_HEAPMAP) {
		heapMap = fopen("heapmap.log", "w+");
	}
	fprintf(stderr, "heap size %zd\n", overallHeapSize);

	heap = (unsigned char*)malloc(heapSize * 8);

	card1 = new CardTable(8,(long)heapSize);  //cardTable to represent 8-bits of the bitmap
	card2 = new CardTable(64,(long)heapSize); //fixing some issues
}

void RealAllocator::freeAllSectors() {
	unsigned int i;
	for (i = 0; i < overallHeapSize; i++) {
		setBitUnused(i);
	}

}

void *RealAllocator::allocate(int size, int lower, int upper) {
	if (size <= 0) {
		return NULL;
	}

	int address = lower;
	int contiguous = 0;
	int i,j,k,bit;
		
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
				        statLiveObjects++;

                        //added by Tristan
                        card1->markCards8(address,size,myHeapBitMap);
                        card2->markCards64(address,size,myHeapBitMap); 
                       
				        return &heap[address];
				    }
			    }
			    k++;
		    }
		    j+=8;
		}
		i+=64;
	}
	return NULL;
}

unsigned int RealAllocator::getHeapIndex(Object *object) {
	// This conversion is only valid because the heap is an array of bytes.
	return (unsigned int) ((char *) object->getAddress() - (char *) heap);
}


void RealAllocator::gcFree(Object* object) {
	int size = object->getHeapSize();
	unsigned int heapIndex = getHeapIndex(object);

	setFree(heapIndex, size);

    card1->unmarkCards(heapIndex,size,myHeapBitMap); //added by Tristan
    card2->unmarkCards(heapIndex,size,myHeapBitMap); //added by Tristan

	statLiveObjects--;
}

RealAllocator::~RealAllocator() {
}

void RealAllocator::freeOldSpace() {
	setFree(oldSpaceStartHeapIndex, oldSpaceEndHeapIndex-oldSpaceStartHeapIndex);
	card1->syncCards8(myHeapBitMap);
	card2->syncCards64(myHeapBitMap);
}

}
