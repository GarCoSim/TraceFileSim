/*
 * TraversalCollector.hpp
 *
 *  Created on: 2015-10-27
 *      Author: GarCoSim
 */

#include "BalancedCollector.hpp"
#define MAXAGE 23 //The biggest age that will be considered
#define MAXAGEP 0.1 //Probability of MAXAGE to be chosen
//#define RAND_MAX 1 //for random numbers to be generated

extern int gLineInTrace;
extern FILE* gLogFile;
extern FILE* gDetLog;

extern FILE* gcFile;

extern clock_t start, stop;

namespace traceFileSimulator {

BalancedCollector::BalancedCollector() {
}


void BalancedCollector::collect(int reason) {
	statCollectionReason = reason;
	stop = clock();
	double elapsed_secs = double(stop - start)/CLOCKS_PER_SEC;
	fprintf(stderr, "GC #%d at %0.3fs", statGcNumber + 1, elapsed_secs);
	
	
	std::vector<Region*> collectionSet = buildCollectionSet();
	
	copy(collectionSet);
	
	
	stop = clock();
	elapsed_secs = double(stop - start)/CLOCKS_PER_SEC;
	fprintf(stderr, " took %0.3fs\n", elapsed_secs);
}

void BalancedCollector::initializeHeap() {
	myAllocator->setNumberOfRegionsHeap(0); 
}

std::vector<Region*> BalancedCollector::buildCollectionSet() {
	std::vector<Region*> allRegions = myAllocator->getRegions();
	std::vector<Region*> collection;
	int i;
	for (i = 0; i < (int)allRegions.size(); i++) {
		Region* currentRegion = allRegions[i];
		//float mortalityRate;
		int regionAge = currentRegion->getAge();
		float probability = regionAge*(MAXAGEP+1)/MAXAGE+1; //linear function passing two points (0,1) (age 0 always selected) and (MAXAGE, MAXAGEP) (there is maximum age which can also be picked with some non-0 probability)
		if ( rand() < probability ) {
			collection.push_back (allRegions[i]);
		}
	}
	return collection;
}

void BalancedCollector::preCollect() {
	start = clock();
	statFreedDuringThisGC = 0;
	statGcNumber++;
}



void BalancedCollector::copy(std::vector<Region*> collectionSet) {
	
	emptyHelpers();

	getCollectionSetRoots(collectionSet);
	
	copyObjects(collectionSet);
}

void BalancedCollector::emptyHelpers() {
	while (!myQueue.empty())
		myQueue.pop();
	while (!myStack.empty())
		myStack.pop();
}

void BalancedCollector::getCollectionSetRoots(std::vector<Region*> collectionSet) {
	Object* currentObj;
	int i, j;
	size_t objectRegion;
	size_t regionSize = myAllocator->getRegionSize();
	unsigned char *heap = myAllocator->getHeap();

	// enqueue all roots, add only objects from collectionSet
	vector<Object*> roots;
	for (i = 0; i < NUM_THREADS; i++) {
		roots = myObjectContainer->getRoots(i);
		for (j = 0; j < (int)roots.size(); j++) {
			currentObj = roots[j];
			if (currentObj) {
				//Add object only if it belongs to region from collectionSet
				objectRegion = currentObj->getRegion((size_t)&heap[0], regionSize);
				myQueue.push(currentObj);
				myStack.push(currentObj);
			}
		}
	}
}


void BalancedCollector::copyObjects(std::vector<Region*> collectionSet) {
	//unsigned int i;
	//Object* currentObj;
	//Region* currentRegion;

	/*
	//breadth first through the tree using a queue
	while (!myQueue.empty()) {
		currentObj = myQueue.front();
		myQueue.pop();
		
		//get actual Obejct Address
		size_t objectAddress = myAllocator->getHeapIndex(currentObj);
		//get regionAddress for the object
		size_t regionAddressForObject;
		
		//Check if Object belongs to a Region from collectionSet
		//size_t regionAddress = currentObj->getRegion();
		void *blaAdress;
		for (i = 0; i < collectionSet.size(); i++) {
			currentRegion = collectionSet[i];
			regionAddress = currentRegion->getAddress();
			if ((size_t)blaAdress == regionAddress) {
				fprintf(stderr, "This is some shit\n");
			}
		}
	}
	*/	
	
	
	//unsigned char *blaHeap = myAllocators[0]->getHeap();
	//size_t regionBla = object->getRegion((size_t)&blaHeap[0], myAllocators[0]->getRegionSize());
		
		/*
		//Stuff from TraversalCollector
		int kids = currentObj->getPointersMax();
		copyAndForwardObject(currentObj);
		currentObj->setAge(currentObj->getAge() + 1);
		for (i = 0; i < kids; i++) {
			child = currentObj->getReferenceTo(i);
			//no matter if the child was processed before or not, add it to the rem set.
			if(child && child->getGeneration() < currentObj->getGeneration()){
				myMemManager->requestRemSetAdd(child);
			}
			if (child && !child->getVisited() && child->getGeneration() <= myGeneration) {
				child->setVisited(true);

				myQueue.push(child);
			}
		}
		*/
	
}

BalancedCollector::~BalancedCollector() {
}

}