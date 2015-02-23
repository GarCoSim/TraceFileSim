/*
 * Simulator.cpp
 *
 *  Created on: Sep 3, 2013
 *      Author: GarCoSim
 */

#include "Simulator.hpp"
#include <string>
#include <stdlib.h>

using namespace std;

extern int gLineInTrace;
extern int gAllocations;

namespace traceFileSimulator {

Simulator::Simulator(char* traceFilePath, int heapSize, int highWatermark, int garbageCollector) {
	myLastStepWorked = 1;
	myTraceFile.open(traceFilePath);
	if(!myTraceFile.good()){
		fprintf(stderr, "File open failed.\n");
		return;
	}

	myMemManager = new MemoryManager(heapSize, highWatermark, garbageCollector);
	counter = 0;
	start = clock();
	seconds = 0;
}

string Simulator::getNextLine(){
	string line = "";
	//try parsing next line
	if(getline(myTraceFile, line)){
		myLastStepWorked = 1;
	} else {
		myLastStepWorked = 0;
	}
	return line;
}

int Simulator::doNextStep(){
	string traceLine = getNextLine();
	gLineInTrace++;
	if (ONE_SECOND_PASSED) {
		start = clock();
		seconds++;
		printf("[%3ds] Line in tracefile: %7d\n", seconds, gLineInTrace);
	}
	if(myLastStepWorked){
		//if content exists, advice the MM(memory manager) to execute
		char firstChar = traceLine.at(0);
		switch(firstChar) {
			case 'w':
				referenceOperation(traceLine);
				break;
			case 'a':
				allocateToRootset(traceLine);
				//next line is a '+', which we skip since it adds the newly created object
				//to the rootset, which already happened in the simulator
				getNextLine();
				gLineInTrace++;
				break;
			case '+':
				//allocateToRootset(traceLine);
				addToRoot(traceLine);
				break;
			case '-':
				deleteRoot(traceLine);
				break;
			default:
				//gLineInTrace++;
			break;
		}

	}

	/*This line calls a garbage collec after each line. Usually useful
	 * in order to analyse the actual heap use of the file.*/
	//myMemManager->evalCollect();

	/*print stats to log file after each line
	 *.. usually used for result collection and comparison*/
	//myMemManager->printStats();


	return 0;
}


int Simulator::lastStepWorked(){
	if(myLastStepWorked == 1){
		return 1;
	}
	return 0;
}


void Simulator::allocateToRootset(string line){
	int thread, id, size, refCount;
	int pos, length;

	pos = line.find('T')+1;
	length = line.find(' ',pos)-pos;
	thread = atoi(line.substr(pos,length).c_str());

	// pos = line.find('R')+1;
	// length = line.find(' ',pos)-pos;
	// rootsetIndex = atoi(line.substr(pos,length).c_str());

	pos = line.find('O')+1;
	length = line.find(' ',pos)-pos;
	id = atoi(line.substr(pos,length).c_str());

	pos = line.find('S')+1;
	length = line.find(' ',pos)-pos;
	size = atoi(line.substr(pos,length).c_str());

	pos = line.find('N')+1;
	length = line.find('\n',pos)-pos;
	refCount = atoi(line.substr(pos,length).c_str());

	myMemManager->allocateObjectToRootset(thread, id, size, refCount);
}

void Simulator::deleteRoot(string line){
	int thread, id;
	int pos, length;

	pos = line.find('T')+1;
	length = line.find(' ',pos)-pos;
	thread = atoi(line.substr(pos,length).c_str());

	pos = line.find('O')+1;
	length = line.find(' ',pos)-pos;
	id = atoi(line.substr(pos,length).c_str());

	myMemManager->requestRootDelete(thread, id);
}

void Simulator::addToRoot(string line){
	int thread, id;
	int pos, length;

	pos = line.find('T')+1;
	length = line.find(' ',pos)-pos;
	thread = atoi(line.substr(pos,length).c_str());

	pos = line.find('O')+1;
	length = line.find(' ',pos)-pos;
	id = atoi(line.substr(pos,length).c_str());

	myMemManager->requestRootAdd(thread, id);
}

void Simulator::allocateObject(string line){
	int thread, parent, parentSlot, id, size, refCount;
	int pos, length;

	pos = line.find('T')+1;
	length = line.find(' ',pos)-pos;
	thread = atoi(line.substr(pos,length).c_str());

	pos = line.find('P')+1;
	length = line.find(' ',pos)-pos;
	parent = atoi(line.substr(pos,length).c_str());

	pos = line.find('#')+1;
	length = line.find(' ',pos)-pos;
	parentSlot = atoi(line.substr(pos,length).c_str());

	pos = line.find('O')+1;
	length = line.find(' ',pos)-pos;
	id = atoi(line.substr(pos,length).c_str());

	pos = line.find('S')+1;
	length = line.find(' ',pos)-pos;
	size = atoi(line.substr(pos,length).c_str());

	pos = line.find('N')+1;
	length = line.find('\n',pos)-pos;
	refCount = atoi(line.substr(pos,length).c_str());

	myMemManager->allocateObject(thread, parent, parentSlot, id, size, refCount);
}

void Simulator::referenceOperation(string line){
	int thread, parentID, parentSlot, childId;
	int pos, length;
	pos = line.find('T')+1;
	length = line.find(' ',pos)-pos;
	thread = atoi(line.substr(pos,length).c_str());

	pos = line.find('P')+1;
	length = line.find(' ',pos)-pos;
	parentID = atoi(line.substr(pos,length).c_str());

	pos = line.find('#')+1;
	length = line.find(' ',pos)-pos;
	parentSlot = atoi(line.substr(pos,length).c_str());

	pos = line.find('O')+1;
	length = line.find('\n',pos)-pos;
	childId = atoi(line.substr(pos,length).c_str());

	myMemManager->setPointer(thread, parentID, parentSlot, childId);
}

void Simulator::printStats(){
	myMemManager->printStats();
}

Simulator::~Simulator() {

}

}
