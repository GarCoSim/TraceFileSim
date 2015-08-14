/*
 * Simulator.hpp
 *
 *  Created on: Sep 3, 2013
 *      Author: GarCoSim
 */

#ifndef SIMULATOR_HPP_
#define SIMULATOR_HPP_
 
#include <stdio.h>
#include <fstream>
#include <ctime>
#include "MemoryManager.hpp"
#include <string>
#include <stdlib.h>

#define ONE_SECOND_PASSED ((double(clock() - start) / CLOCKS_PER_SEC) >= 1.0f)

using namespace std;

namespace traceFileSimulator {

typedef struct TraceFileLine {
	char type;
	int classID;
	int fieldIndex;
	int fieldOffset;
	int fieldSize;
	int fieldType;
	int objectID;
	int parentID;
	int parentSlot;
	int maxPointers;
	int size;
	int threadID;
} TraceFileLine;

class Simulator {
public:
	Simulator(char* traceFilePath, int heapSize, int highWatermark, int garbageCollector, int traversal, int allocator);
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
	MemoryManager* myMemManager;
	
	//debug
	int counter;
	clock_t start;
	int seconds;
};

} 
#endif /* SIMULATOR_HPP_ */
