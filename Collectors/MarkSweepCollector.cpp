/*
 * MarkSweepCollector.cpp
 *
 *  Created on: 2013-09-04
 *      Author: GarCoSim
 */

#include "Collector.hpp"
#include "MarkSweepCollector.hpp"
#include "../Allocators/Allocator.hpp"
#include "../Main/ObjectContainer.hpp"
#include "../Main/MemoryManager.hpp"
#include <sstream>
#include <ctime>

extern TRACE_FILE_LINE_SIZE gLineInTrace;
extern FILE* gLogFile;
extern FILE* gDetLog;
extern int lockNumber;
extern int countRoots;
extern int countTraversalDepth;

FILE* gcFile;

clock_t start, stop;

namespace traceFileSimulator {

MarkSweepCollector::MarkSweepCollector() {
}

/** Argument indicates the reason for collection: 0 - unknown, 1 - failed alloc, 2 - high watermark
 *
 */
void MarkSweepCollector::collect(int reason) {
	if (lockNumber != 0) {
		fprintf(stderr, "GC triggered and lockNumber is not zero: %i\n", lockNumber);
	} 
	statCollectionReason = reason;
	stop = clock();
	double elapsed_secs = double(stop - start)/CLOCKS_PER_SEC;

	traversalDepthObjects.clear();
	traversalDepth.clear();
	rootObjects.clear();
	amountRootObjects = 0;
	amountOtherObjects = 0;

	fprintf(stderr, "GC #%d at %0.3fs at line %d", statGcNumber + 1, elapsed_secs, gLineInTrace);
	preCollect();
	mark();
	sweep();

	if (countTraversalDepth)
		printTraversalDepthStats();

	if (countRoots)
		printRootCountStats();
	/* Uncomment this to enable compaction
	if (statFreedDuringThisGC > 0) {
		compact();
		updatePointers();
	}
	*/

	postCollect();

	stop = clock();
	elapsed_secs = double(stop - start)/CLOCKS_PER_SEC;
	fprintf(stderr, " took %0.3fs\n", elapsed_secs);
}

void MarkSweepCollector::initializeHeap() {
	myAllocator->setHalfHeapSize(false);
}

/** Marks all objects, starting at the root set
 *
 */
void MarkSweepCollector::mark() {
	initializeMarkPhase();
	enqueueAllRoots();

	amountRootObjects = traversalDepthObjects.size();

	//breadth first through the tree
	size_t i;
	int childDepth;
	while (!myQueue.empty()) {
		Object* currentObj = myQueue.front();
		myQueue.pop();
		Object* child;
		size_t kids = currentObj->getPointersMax();
		currentObj->setAge(currentObj->getAge() + 1);
		for (i = 0; i < kids; i++) {
			child = currentObj->getReferenceTo(i);
			//no matter if the child was processed before or not, add it to the rem set.
			if(child && child->getGeneration() < currentObj->getGeneration())
				myMemManager->requestRemSetAdd(child);
			if (child && !child->getVisited() && child->getGeneration() <= myGeneration) {
				child->setVisited(true);
				myQueue.push(child);

				if (countTraversalDepth) {
					childDepth = (traversalDepthObjects.find(currentObj->getID())->second) + 1;
					traversalDepthObjects.insert( std::pair<int,int>(child->getID(), childDepth));
					if (traversalDepth.find(childDepth) != traversalDepth.end())
						traversalDepth.at(childDepth) = traversalDepth.at(childDepth) + 1;
					else
						traversalDepth.insert(std::pair<int, int>(childDepth, 1));
				}
			}
		}
	}
	amountOtherObjects = traversalDepthObjects.size() - amountRootObjects;
}

/** Deletes all unmarked objects in all generations
 *
 */
void MarkSweepCollector::sweep() {
	Object* currentObj;
	int gGC;
	size_t heapPosition= 0;
	RawObject* raw;

	while(heapPosition < myAllocator->getHeapSize()){

		raw = (RawObject *)myAllocator->getNextObjectAddress(heapPosition);

		if(raw!=NULL){
			currentObj = raw->associatedObject;
			if(!currentObj){
				std::stringstream ss;
				ss << "Object does not exist at heap position " << heapPosition << ". The algorithm for stepping through the heap has failed.\n";
				ERRMSG(ss.str().c_str());
				throw 19;
			}

			heapPosition += myAllocator->getSpaceToNextObject(heapPosition);
			heapPosition += currentObj->getHeapSize();

			if(myGeneration == GENERATIONS -1){
				gGC = 1;
			} else {
				gGC = 0;
			}

			//TODO: remove null checks and test if the code still passes tests. IDE says it will never be null;
			if (currentObj && !currentObj->getVisited()) {
				myMemManager->requestDelete(currentObj, gGC);
				statFreedObjects++;
				statFreedDuringThisGC++;
			}
		}
		else{
			break;
		}
	}
}

/** Adds all objects from all roots to the queue
 *
 */
void MarkSweepCollector::enqueueAllRoots() {
	Object* currentObj;
	size_t i, j;
	if (myGeneration == GENERATIONS - 1) {
		//we are performing a global GC and can use it to fix possible rem set problems
		//we clear all rem sets and fill them again while performing the marking
		myMemManager->clearRemSets();

		vector<Object*> roots;
		for (i = 0; i < NUM_THREADS; i++) {
			roots = myObjectContainer->getRoots(i);
			for (j = 0; j < roots.size(); j++) {
				currentObj = roots[j];

				if (currentObj && (countRoots) ) {
					if (rootObjects.find(currentObj->getID()) != rootObjects.end())
						rootObjects.at(currentObj->getID()) = rootObjects.at(currentObj->getID()) + 1;
					else
						rootObjects.insert(std::pair<int, int>(currentObj->getID(),1));
				}

				if (currentObj && !currentObj->getVisited()) {
					currentObj->setVisited(true);
					//add to rem set if the root is in a younger generation.
					if (currentObj->getGeneration() < myGeneration) {
						myMemManager->requestRemSetAdd(currentObj);
					}
					myQueue.push(currentObj);

					if (countTraversalDepth) {
						traversalDepthObjects.insert( std::pair<int,int>(currentObj->getID(), 1) );
						if (traversalDepth.find(1) != traversalDepth.end())
							traversalDepth.at(1) = traversalDepth.at(1) + 1;
						else
							traversalDepth.insert(std::pair<int, int>(1,1));
					}
				}
			}
		}
	} else {
		for (j = 0; j < myObjectContainer->getGenRootSize(); j++) {
			currentObj = myObjectContainer->getGenRoot(j);

			if (currentObj && (countRoots) ) {
				if (rootObjects.find(currentObj->getID()) != rootObjects.end())
					rootObjects.at(currentObj->getID()) = rootObjects.at(currentObj->getID()) + 1;
				else
					rootObjects.insert(std::pair<int, int>(currentObj->getID(),1));
			}

			if (currentObj && !currentObj->getVisited()) {
				currentObj->setVisited(true);
				myQueue.push(currentObj);

				if (countTraversalDepth) {
					traversalDepthObjects.insert( std::pair<int,int>(currentObj->getID(), 1) );
					if (traversalDepth.find(1) != traversalDepth.end())
						traversalDepth.at(1) = traversalDepth.at(1) + 1;
					else
						traversalDepth.insert(std::pair<int, int>(1,1));
				}
			}
		}
	}
}









MarkSweepCollector::~MarkSweepCollector() {
}

}
