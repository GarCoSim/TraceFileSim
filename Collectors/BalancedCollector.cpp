/*
 * TraversalCollector.hpp
 *
 *  Created on: 2015-10-27
 *      Author: GarCoSim
 */

#include "BalancedCollector.hpp"
#define MAXAGE 23 //The biggest age that will be considered
#define MAXAGEP 0.1 //Probability of MAXAGE to be chosen
#define RAND_MAX 1 //for random numbers to be generated

extern int gLineInTrace;
extern FILE* gLogFile;
extern FILE* gDetLog;

extern FILE* gcFile;

extern clock_t start, stop;

namespace traceFileSimulator {

BalancedCollector::BalancedCollector() {
}

/**
 * Argument indicates the reason for collection: 0 - unknown, 1 - failed alloc, 2 - high watermark
 */
void BalancedCollector::collect(int reason) {
	statCollectionReason = reason;
	stop = clock();
	double elapsed_secs = double(stop - start)/CLOCKS_PER_SEC;
	fprintf(stderr, "GC #%d at %0.3fs", statGcNumber + 1, elapsed_secs);
	//collectionSet();
	//Other things. TBD
	stop = clock();
	elapsed_secs = double(stop - start)/CLOCKS_PER_SEC;
	fprintf(stderr, " took %0.3fs\n", elapsed_secs);
}

void BalancedCollector::initializeHeap() {
	myAllocator->setNumberOfRegionsHeap(0); 
}

vector<Region*> BalancedCollector::collectionSet() {
	vector<Region*> allRegions = myAllocator->getRegions();
	vector<Region*> collection;
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

BalancedCollector::~BalancedCollector() {
}

}