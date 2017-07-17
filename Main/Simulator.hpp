/*
 * Simulator.hpp
 *
 *  Created on: Sep 3, 2013
 *      Author: GarCoSim
 */

#ifndef SIMULATOR_HPP_
#define SIMULATOR_HPP_

#include <fstream>
#include <ctime>
#include "Optional.cpp"

#define ONE_SECOND_PASSED ((double(clock() - start) / CLOCKS_PER_SEC) >= 1.0f)

using namespace std;

namespace traceFileSimulator {

class MemoryManager;

/** Structure which stores information parsed from reading a line in a trace file
 *
 */
typedef struct TraceFileLine {
	Optional<char> type;
	Optional<int> classID;
	Optional<int> fieldIndex;
	Optional<int> fieldOffset;
	Optional<int> fieldSize;
	Optional<int> fieldType;
	Optional<int> objectID;
	Optional<int> parentID;
	Optional<size_t> parentSlot;
	Optional<size_t> maxPointers;
	Optional<size_t> size;
	Optional<int> threadID;
} TraceFileLine;

class Simulator {
public:
    Simulator(char* traceFilePath, size_t heapSize, size_t maxHeapSize, int highWatermark, int garbageCollector, int traversal, int allocator, int writebarrier, int finalGC);
	virtual ~Simulator();
	int lastStepWorked();
	int doNextStep();
	void printStats();
	void lastStats();

private:
	void getNextLine(TraceFileLine *line);
	void initializeTraceFileLine(TraceFileLine *line);
	void allocateToRootset(TraceFileLine line);
	void referenceOperation(TraceFileLine line);
	void deleteRoot(TraceFileLine line);
	void addToRoot(TraceFileLine line);
	void referenceOperationClassField(TraceFileLine line);
	void readOperation(TraceFileLine line);
	void storeOperation(TraceFileLine line);

	ifstream myTraceFile;

	int myLastStepWorked;
	int myFinalGC;
	MemoryManager* myMemManager;

	//debug
	int counter;
	clock_t start;
	int seconds;
};

}
#endif /* SIMULATOR_HPP_ */
