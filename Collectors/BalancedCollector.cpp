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
extern FILE* balancedLogFile;

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
	
	
	buildCollectionSet();
	
	copy();
	
	
	stop = clock();
	elapsed_secs = double(stop - start)/CLOCKS_PER_SEC;
	fprintf(stderr, " took %0.3fs\n", elapsed_secs);
}

void BalancedCollector::initializeHeap() {
	myAllocator->setNumberOfRegionsHeap(0); 
}

void BalancedCollector::buildCollectionSet() {
	fprintf(balancedLogFile, "Building collection set:\n");
	std::vector<Region*> allRegions = myAllocator->getRegions();
	myCollectionSet.clear();
	std::vector<unsigned int> edenRegions = myAllocator->getEdenRegions();
	unsigned int i, j;
	
	//For now: Add all and only the Eden Regions
	for (i = 0; i < allRegions.size(); i++) {
		for (j = 0; j < edenRegions.size(); j++) {
			if (i == edenRegions[j]) {
				myCollectionSet.push_back (i);
				fprintf(balancedLogFile, "Added region %i to collection set\n", i);
			}
		}
		
		/*
		Region* currentRegion = allRegions[i];
		
		//linear function passing two points (0,1) (age 0 always selected) and (MAXAGE, MAXAGEP) 
		//there is maximum age which can also be picked with some non-0 probability
		float mortalityRate;
		int regionAge = currentRegion->getAge();
		float probability = regionAge*(MAXAGEP+1)/MAXAGE+1; 
		if ( rand() < probability ) {
			myCollectionSet.push_back (i);
		}
		*/
	}
}

void BalancedCollector::preCollect() {
	start = clock();
	statFreedDuringThisGC = 0;
	statGcNumber++;
}



void BalancedCollector::copy() {
	fprintf(balancedLogFile, "Start copying\n");
	emptyHelpers();

	getCollectionSetRoots();
	
	copyObjects();
}

void BalancedCollector::emptyHelpers() {
	while (!myQueue.empty())
		myQueue.pop();
	while (!myStack.empty())
		myStack.pop();
}

void BalancedCollector::getCollectionSetRoots() {
	fprintf(balancedLogFile, "Get Root Objects from collection set\n");
	Object* currentObj;
	int i;
	unsigned int objectRegion, k, j;

	// enqueue all roots, add only objects from collectionSet
	vector<Object*> roots;
	for (i = 0; i < NUM_THREADS; i++) {
		roots = myObjectContainer->getRoots(i);
		fprintf(balancedLogFile, "Found %zu root objects\n", roots.size());
		
		for (j = 0; j < roots.size(); j++) {
			currentObj = roots[j];
			if (currentObj && !currentObj->getVisited()) {
				currentObj->setVisited(true);
				//Add object only if it belongs to region from collectionSet
				fprintf(balancedLogFile, "Current object: ID = %i. ", currentObj->getID());
				objectRegion = myAllocator->getObjectRegion(currentObj);
				fprintf(balancedLogFile, " Appropriate object Region: %u\n", objectRegion);
				for (k = 0; k < myCollectionSet.size(); k++) {
					if (objectRegion == myCollectionSet[k]) {
						myQueue.push(currentObj);
						myStack.push(currentObj);
						fprintf(balancedLogFile, "Added object %i.\n\n", currentObj->getID());
					}
				}
			}
		}
	}
}


void BalancedCollector::copyObjects() {
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
	
	
	//unsigned char *blaHeap = myAllocator->getHeap();
	//size_t regionBla = object->getRegion((size_t)&blaHeap[0], myAllocator->getRegionSize());
		
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