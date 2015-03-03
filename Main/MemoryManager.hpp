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
#include "../Allocators/Allocator.hpp"
#include "../Allocators/RealAllocator.hpp"
#include "../Allocators/SimulatedAllocator.hpp"
#include "../Collectors/Collector.hpp"
#include "../Collectors/MarkSweepCollector.hpp"
#include "../Collectors/CopyingCollector.hpp"
#include "../Collectors/TraversalCollector.hpp"
#include "../defines.hpp"

namespace traceFileSimulator {

class MemoryManager {
public:
	MemoryManager(int heapSize, int highWatermark, int collector, int traversal, int allocator);
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
	void initGarbageCollectors(int highWatermark);
	size_t allocate(int size, int generation);
	void addRootToContainers(Object* object, int thread, int rootsetIndex);
	void addToContainers(Object* object);
	size_t shift(int size);
	
	allocatorEnum _allocator;
	collectorEnum _collector;
	traversalEnum _traversal;

	Allocator* myAllocators[GENERATIONS];
	ObjectContainer* myObjectContainers[GENERATIONS];
	Collector* myGarbageCollectors[GENERATIONS];
	int stats[GENERATIONS];

};

} 
#endif /* MEMORYMANAGER_HPP_ */
