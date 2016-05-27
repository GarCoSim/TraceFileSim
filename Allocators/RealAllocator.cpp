/*
 * RealAllocator.cpp
 *
 *  Created on: 2015-03-04
 *      Author: GarCoSim
 */

#include "RealAllocator.hpp"
#include "../Main/CardTable.hpp" 
#include "../Main/Object.hpp"

extern int gLineInTrace;

extern double totTime; //added by Tristan


using namespace std;

namespace traceFileSimulator {

RealAllocator::RealAllocator() {
}

bool RealAllocator::isRealAllocator() {
	return true;
}

void RealAllocator::initializeHeap(size_t heapSize) {
	Allocator::initializeHeap(heapSize);

	card1 = new CardTable(8,(long)heapSize);  //cardTable to represent 8-bits of the bitmap
	card2 = new CardTable(64,(long)heapSize); //fixing some issues
}

void *RealAllocator::allocate(size_t size, size_t lower, size_t upper) {
	if (size <= 0) {
		return NULL;
	}

	size_t address = lower;
	size_t contiguous = 0;
	size_t i,j,k,bit;
		
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



void RealAllocator::gcFree(Object* object) {
	size_t size = object->getHeapSize();
	size_t heapIndex = getHeapIndex(object);

	setFree(heapIndex, size);

    card1->unmarkCards(heapIndex,size,myHeapBitMap); //added by Tristan
    card2->unmarkCards(heapIndex,size,myHeapBitMap); //added by Tristan
}

RealAllocator::~RealAllocator() {
}

void RealAllocator::freeOldSpace() {
	Allocator::freeOldSpace();
	card1->syncCards8(myHeapBitMap);
	card2->syncCards64(myHeapBitMap);
}

}
