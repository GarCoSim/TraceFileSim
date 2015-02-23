/*
 * MemoryManager.hpp
 *
 *  Created on: 2013-09-03
 *      Author: GarCoSim
 */

#ifndef MEMORYMANAGER_HPP_
#define MEMORYMANAGER_HPP_

#include "Object.hpp"
#include "ObjectContainer.hpp"
#include "Allocator.hpp"
#include "Collector.hpp"
#include "MarkSweepCollector.hpp"
#include "CopyingCollector.hpp"
#include "../defines.hpp"

namespace traceFileSimulator {

class MemoryManager {
public:
	MemoryManager(int heapSize, int highWatermark, int collector);
	virtual ~MemoryManager();
	//operations possible from trace file
	int allocateObjectToRootset(int thread, int id, int size, int refCount);
	int allocateObject(int thread, int parentID, int parentSlot, int id, int size, int refCount);
	int requestRootDelete(int thread, int id);
	int requestRootAdd(int thread, int id);
	int setPointer(int thread, int parentID, int parentSlot, int childID);
	void requestDelete(Object* object, int gGC);
	void requestFree(Object* object);
	void requestReallocate(Object* object);
	void requestResetAllocationPointer(int generation);
	int requestPromotion(Object* object);
	void printStats();
	void statBeforeCompact(int myGeneration);
	void statAfterCompact(int myGeneration);
	int evalCollect();
	void createRemSetEntries(Object* parent, Object* child);
	void createRemSetEntriyRoot(Object* object);
	void clearRemSets();
	void requestRemSetAdd(Object* currentObj);

private:
	int* computeHeapsizes(int heapSize);
	void initAllocators(int heapsize);
	void initContainers();
	void initGarbageCollectors(int highWatermark, int collector);
	int allocate(int size, int generation);
	void addRootToContainers(Object* object, int thread, int rootsetIndex);
	void addToContainers(Object* object);
	int shift(int size);
	
	Allocator* myAllocators[GENERATIONS];
	ObjectContainer* myObjectContainers[GENERATIONS];
	Collector* myGarbageCollectors[GENERATIONS];
	int stats[GENERATIONS];

};

} 
#endif /* MEMORYMANAGER_HPP_ */
