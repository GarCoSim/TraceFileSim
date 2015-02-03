/*
 * Simulator.h
 *
 *  Created on: Sep 3, 2013
 *      Author: kons
 */

#ifndef SIMULATOR_H_
#define SIMULATOR_H_
#include <stdio.h>
#include <fstream>
#include "MemoryManager.h"

using namespace std;
namespace traceFileSimulator {

class Simulator {
public:
	Simulator(char* traceFilePath, int heapSize, int highWatermark);
	virtual ~Simulator();
	int lastStepWorked();
	int doNextStep();
	void printStats();
private:
	string getNextLine();
	void allocateToRootset(string line);
	void allocateObject(string line);
	void referenceOperation(string line);
	void deleteRoot(string line);
	void addToRoot(string line);
	ifstream myTraceFile;
	int myLastStepWorked;
	int outputCounter;
	MemoryManager* myMemManager;
	//debug
	int counter;
};//class Sumulator
} /* namespace gcKons */
#endif /* SIMULATOR_H_ */
