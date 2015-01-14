/*
 * Simulator.cpp
 *
 *  Created on: Sep 3, 2013
 *      Author: kons
 */

#include "Simulator.h"
#include <string>
#include <stdlib.h>

using namespace std;

extern int gLineInTrace;
extern int gAllocations;

namespace traceFileSimulator {




Simulator::Simulator(char* traceFilePath, int heapSize, int highWatermark) {
	myLastStepWorked = 1;
	myTraceFile.open(traceFilePath);
	if(!myTraceFile.good()){
		fprintf(stderr, "File open failed.\n");
		return;
	}

	myMemManager = new MemoryManager(heapSize, highWatermark);
	counter = 0;
	outputCounter = 0;
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
	outputCounter--;
	if(outputCounter <= 0){
		printf("\nLine:%7d\n",gLineInTrace);
		outputCounter = 10000;
	}
//	if(gLineInTrace > 66000){
//		printf("Line:%7d\n",gLineInTrace);
//	}
	if(myLastStepWorked){
		//if content exists, advice the MM(memory manager) to execute
		char firstChar = traceLine.at(0);
		if(firstChar == 'r'){
			referenceOperation(traceLine);
		} else if (firstChar == 'a'){//allocation
			gAllocations++;
			//allocation operation
			//if we have a Root info, it is a rootset allocation
			if(traceLine.find('R') != string::npos){
				allocateToRootset(traceLine);
			} else {
				allocateObject(traceLine);
			}
		}else {//root delete
			deleteRoot(traceLine);
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
	int thread, rootsetIndex, id, size, refCount;
	int pos, length;

	pos = line.find('T')+1;
	length = line.find(' ',pos)-pos;
	thread = atoi(line.substr(pos,length).c_str());

	pos = line.find('R')+1;
	length = line.find(' ',pos)-pos;
	rootsetIndex = atoi(line.substr(pos,length).c_str());

	pos = line.find('O')+1;
	length = line.find(' ',pos)-pos;
	id = atoi(line.substr(pos,length).c_str());

	pos = line.find('S')+1;
	length = line.find(' ',pos)-pos;
	size = atoi(line.substr(pos,length).c_str());

	pos = line.find('N')+1;
	length = line.find('\n',pos)-pos;
	refCount = atoi(line.substr(pos,length).c_str());

	myMemManager->allocateObjectToRootset(thread, rootsetIndex, id, size, refCount);
}

void Simulator::deleteRoot(string line){
	int thread, root;
	int pos, length;

	pos = line.find('T')+1;
	length = line.find(' ',pos)-pos;
	thread = atoi(line.substr(pos,length).c_str());

	pos = line.find('R')+1;
	length = line.find(' ',pos)-pos;
	root = atoi(line.substr(pos,length).c_str());

	myMemManager->requestRootDelete(thread, root);
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

} /* namespace gcKons */
