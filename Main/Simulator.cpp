/*
 * Simulator.cpp
 *
 *  Created on: Sep 3, 2013
 *      Author: GarCoSim
 */

#include "Simulator.hpp"

using namespace std;

extern int gLineInTrace;
extern int gAllocations;
extern int forceAGCAfterEveryStep;

namespace traceFileSimulator {

Simulator::Simulator(char* traceFilePath, int heapSize, int highWatermark, int garbageCollector, int traversal, int allocator) {
	myLastStepWorked = 1;
	myTraceFile.open(traceFilePath);
	if(!myTraceFile.good()){
		fprintf(stderr, "File open failed.\n");
		exit(1);
	}

	myMemManager = new MemoryManager(heapSize, highWatermark, garbageCollector, traversal, allocator);

	if (!myMemManager->loadClassTable((string)traceFilePath))
		fprintf(stdout, "No class table found\n");

	counter = 0;
	start = clock();
	seconds = 0;
}

string Simulator::getNextLine(){
	string line = "";
	//try parsing next line, we skip empty lines

	if (myTraceFile.eof()) {
		myLastStepWorked = 0;
		return line;
	}

	do {
		if(getline(myTraceFile, line)){
			myLastStepWorked = 1;
		} else {
			myLastStepWorked = 0;
		}
		gLineInTrace++;
	} while (line.size() == 0 && !myTraceFile.eof());

	return line;
}

void Simulator::lastStats() {
	myMemManager->lastStats();
}

int Simulator::parseAttributeFromTraceLine(char attributeIdentifier, string line) {
	int pos, length;
	pos = line.find(attributeIdentifier)+1;
	length = min(line.find(' ',pos), line.find('\n',pos)) - pos;
	return atoi(line.substr(pos,length).c_str());
}

int Simulator::doNextStep(){
	string traceLine = getNextLine();
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
				// error checking
				if(  ( (int)traceLine.find('T') == -1 ) || ( (int)traceLine.find('P') == -1 ) || ( (int)traceLine.find('#') == -1 ) || 
					( (int)traceLine.find('O') == -1 ) || ( ( (int)traceLine.find('F') == -1 ) && ( (int)traceLine.find('I') == -1 ) ) || 
					( (int)traceLine.find('S') == -1 ) || ( (int)traceLine.find('V') == -1 ) ){
					printf("Prefix error in line: %d\n", gLineInTrace);
					exit(1);
				}

				//referenceOperation(traceLine);
				break;
			case 'a':
				// error checking
				if(  ( (int)traceLine.find('T') == -1 ) || ( (int)traceLine.find('O') == -1 ) || ( (int)traceLine.find('S') == -1 ) || 
					( (int)traceLine.find('N') == -1 ) || ( (int)traceLine.find('C') == -1 ) ){
					printf("Prefix error in line: %d\n", gLineInTrace);
					exit(1);
				}
				//allocateToRootset(traceLine);
				//next line is a '+', which we skip since it adds the newly created object
				//to the rootset, which already happened in the simulator
				getNextLine();
				break;
			case '+':
				// error checking
				if(  ( (int)traceLine.find('T') == -1 ) || ( (int)traceLine.find('O') == -1 ) ){
					printf("Prefix error in line: %d\n", gLineInTrace);
					exit(1);
				}
				//allocateToRootset(traceLine);
				//addToRoot(traceLine);
				break;
			case '-':
				// error checking
				if(  ( (int)traceLine.find('T') == -1 ) || ( (int)traceLine.find('O') == -1 ) ){
					printf("Prefix error in line: %d\n", gLineInTrace);
					exit(1);
				}
				//deleteRoot(traceLine);
				break;
			case 'c': // for now we ignore the class option
				// error checking
				if(  ( (int)traceLine.find('T') == -1 ) || ( (int)traceLine.find('C') == -1 ) || ( (int)traceLine.find('F') == -1 ) || 
					( (int)traceLine.find('O') == -1 ) || ( (int)traceLine.find('S') == -1 ) || ( (int)traceLine.find('V') == -1 ) ){
					printf("Prefix error in line: %d\n", gLineInTrace);
					exit(1);
				}
				//referenceOperationClassField(traceLine);
				break;
			case 'r': // for now we ignore the class option
				// error checking
				if(  ( (int)traceLine.find('T') == -1 ) || ( ( (int)traceLine.find('C') == -1 ) && ( (int)traceLine.find('O') == -1 ) ) || 
					(( (int)traceLine.find('F') == -1 ) && ( (int)traceLine.find('I') == -1 )) || ( (int)traceLine.find('S') == -1 ) || 
					( (int)traceLine.find('V') == -1 ) ){
					printf("Prefix error in line: %d\n", gLineInTrace);
					exit(1);
				}
				//readOperation(traceLine);
				break;
			case 's': // for now we ignore the class option
				if(  ( (int)traceLine.find('T') == -1 ) || ( ( (int)traceLine.find('C') == -1 ) && ( (int)traceLine.find('P') == -1 ) ) || 
					(( (int)traceLine.find('F') == -1 ) && ( (int)traceLine.find('I') == -1 )) || ( (int)traceLine.find('S') == -1 ) || 
					( (int)traceLine.find('V') == -1 ) ){
					printf("Prefix error in line: %d\n", gLineInTrace);
					exit(1);
				}
				//storeOperation(traceLine);
				break;	

			default:
				//gLineInTrace++;
			break;
		}

	}
	// commented by mazder
	//if (forceAGCAfterEveryStep) 
	//	myMemManager->forceGC();

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
	int classID;

	thread = parseAttributeFromTraceLine('T', line);
	id = parseAttributeFromTraceLine('O', line);
	refCount = parseAttributeFromTraceLine('N', line);
	size = parseAttributeFromTraceLine('S', line);
	size += sizeof(Object);

	if (myMemManager->hasClassTable()) {
		classID = parseAttributeFromTraceLine('C', line);
	} else
		classID = 0;

	myMemManager->allocateObjectToRootset(thread, id, size, refCount, classID);
}

void Simulator::deleteRoot(string line){
	int thread, id;
	size_t pos;

	thread = parseAttributeFromTraceLine('T', line);
	id = parseAttributeFromTraceLine('O', line);

	// for now we skip the removal of classes in the tracefile, needs to be addressed in future releases
	pos = line.find('C');
	if (pos != string::npos)
		return;

	myMemManager->requestRootDelete(thread, id);
}

void Simulator::addToRoot(string line){
	int thread, id;

	thread = parseAttributeFromTraceLine('T', line);
	id = parseAttributeFromTraceLine('O', line);

	myMemManager->requestRootAdd(thread, id);
}

void Simulator::allocateObject(string line){
	int thread, parent, parentSlot, id, size, refCount;
	int classID;

	thread = parseAttributeFromTraceLine('T', line);
	parent = parseAttributeFromTraceLine('P', line);
	parentSlot = parseAttributeFromTraceLine('#', line);
	id = parseAttributeFromTraceLine('O', line);
	refCount = parseAttributeFromTraceLine('N', line);
	size = parseAttributeFromTraceLine('S', line);
	size += sizeof(Object);

	//check objectID for NULL object.
	if(id == 0){
		fprintf(stderr, "ERROR (%d): Object ID 0 is reserved for a NULL pointer.\n", gLineInTrace);
		exit(1);
	}

	if (myMemManager->hasClassTable()) {
		classID = parseAttributeFromTraceLine('C', line);
	} else
		classID = 0;

	myMemManager->allocateObject(thread, parent, parentSlot, id, size, refCount, classID);
}

void Simulator::referenceOperation(string line){
	int thread, parentID, parentSlot, childId;
	int pos, length;

	// added by mazder (the following information is added in TraceFile-3.0)
	int fieldOffset;  // it is offest of the parentSlot when single object 
	int fieldIndex;	  // same as the parent slot when array object	
	int fieldSize;   // pointer size
	int fieldType;   // 1 for volatile or 0 for non-volatile
	bool offsetFlag = false; 	 // to decide either offest or index is given   
	//

	thread = parseAttributeFromTraceLine('T', line);
	parentID = parseAttributeFromTraceLine('P', line);
	parentSlot = parseAttributeFromTraceLine('#', line);
	childId = parseAttributeFromTraceLine('O', line);

	myMemManager->setPointer(thread, parentID, parentSlot, childId);

	// added by mazder 
	// In trace file there is either 'F'/'I' in the 'w' line
	pos = line.find('F');
	if(pos != -1){
		pos = pos+1;
		length = line.find('\n',pos)-pos;
		fieldOffset = atoi(line.substr(pos,length).c_str());
		offsetFlag = true;
	}
	if(!offsetFlag){
		pos = line.find('I')+1;
		length = line.find('\n',pos)-pos;
		fieldIndex = atoi(line.substr(pos,length).c_str());
	}

	pos = line.find('S')+1;
	length = line.find('\n',pos)-pos;
	fieldSize = atoi(line.substr(pos,length).c_str());

	pos = line.find('V')+1;
	length = line.find('\n',pos)-pos;
	fieldType = atoi(line.substr(pos,length).c_str());


	if(offsetFlag){
		/* when fieldOffset is given */
	}
	else{
		/* when fieldIndex is given */
	}

}

// Added by Mazder

void Simulator::referenceOperationClassField(string line){
	int thread, classID, objectID;
	int pos, length;
	int fieldOffset;  // offest of the reference slot 
	int fieldSize;   // pointer size
	int fieldType;   // 1 for volatile or 0 for non-volatile

	pos = line.find('T')+1;
	length = line.find(' ',pos)-pos;
	thread = atoi(line.substr(pos,length).c_str());

	pos = line.find('C')+1;
	length = line.find(' ',pos)-pos;
	classID = atoi(line.substr(pos,length).c_str());

	pos = line.find('F')+1;
	length = line.find(' ',pos)-pos;
	fieldOffset = atoi(line.substr(pos,length).c_str());

	pos = line.find('O')+1;
	length = line.find('\n',pos)-pos;
	objectID = atoi(line.substr(pos,length).c_str());

	pos = line.find('S')+1;
	length = line.find('\n',pos)-pos;
	fieldSize = atoi(line.substr(pos,length).c_str());

	pos = line.find('V')+1;
	length = line.find('\n',pos)-pos;
	fieldType = atoi(line.substr(pos,length).c_str());

	/* Insert code here to store object reference into a class, when only fieldOffest of the reference slot is given*/

}
void Simulator::readOperation(string line){
	int thread, classID, objectID;
	int pos, length;
	int fieldOffset;  // offest of the reference slot
	int fieldIndex;   // index of the field in the case of array 	
	int fieldSize;   // pointer/primitive field size
	int fieldType;   // 1 for volatile or 0 for non-volatile
	bool staticFlag = false; 	 // To decide reading is either from a class field ( static field) or an object field    
	bool offsetFlag = false; 	 // to decide either offest or index is given

	pos = line.find('T')+1;
	length = line.find(' ',pos)-pos;
	thread = atoi(line.substr(pos,length).c_str());

	pos = line.find('C');
	if(pos != -1){
		pos = pos+1;
		length = line.find(' ',pos)-pos;
		classID = atoi(line.substr(pos,length).c_str());
		staticFlag = true;
	}
	if(!staticFlag){
		pos = line.find('O')+1;
		length = line.find('\n',pos)-pos;
		objectID = atoi(line.substr(pos,length).c_str());
	}

	pos = line.find('F');
	if(pos != -1){
		pos = pos+1;
		length = line.find('\n',pos)-pos;
		fieldOffset = atoi(line.substr(pos,length).c_str());
		offsetFlag = true;
	}
	if(!offsetFlag){
		pos = line.find('I')+1;
		length = line.find('\n',pos)-pos;
		fieldIndex = atoi(line.substr(pos,length).c_str());
	}

	pos = line.find('S')+1;
	length = line.find('\n',pos)-pos;
	fieldSize = atoi(line.substr(pos,length).c_str());

	pos = line.find('V')+1;
	length = line.find('\n',pos)-pos;
	fieldType = atoi(line.substr(pos,length).c_str());

	if(staticFlag){
		/* read access to class field */
		if(offsetFlag){
			/* when fieldoffset is given */ 
		}
		else{
			/* when fieldIndex is given */ 
		}

	}
	else{
		/* read access to object field */
		if(offsetFlag){
			/* when fieldoffset is given */ 
		}
		else{
			/* when fieldoffset is given */ 
		}
	}

}

/* This is to store static primitive field in class and primitive field in an object */

void Simulator::storeOperation(string line){
	int thread, classID, objectID;
	int pos, length;
	int fieldOffset;  // offest of the reference slot
	int fieldIndex;   // index of the field in the case of array 	
	int fieldSize;   // pointer size
	int fieldType;   // 1 for volatile or 0 for non-volatile
	bool staticFlag = false; 	 // To decide reading is either from a class field ( static field) or an object field    
	bool offsetFlag = false; 	 // to decide either offest or index is given

	pos = line.find('T')+1;
	length = line.find(' ',pos)-pos;
	thread = atoi(line.substr(pos,length).c_str());

	pos = line.find('C');
	if(pos != -1){
		pos = pos+1;
		length = line.find(' ',pos)-pos;
		classID = atoi(line.substr(pos,length).c_str());
		staticFlag = true;
	}
	if(!staticFlag){
		pos = line.find('P')+1;
		length = line.find('\n',pos)-pos;
		objectID = atoi(line.substr(pos,length).c_str());
	}

	pos = line.find('F');
	if(pos != -1){
		pos = pos+1;
		length = line.find('\n',pos)-pos;
		fieldOffset = atoi(line.substr(pos,length).c_str());
		offsetFlag = true;
	}
	if(!offsetFlag){
		pos = line.find('I')+1;
		length = line.find('\n',pos)-pos;
		fieldIndex = atoi(line.substr(pos,length).c_str());
	}

	pos = line.find('S')+1;
	length = line.find('\n',pos)-pos;
	fieldSize = atoi(line.substr(pos,length).c_str());

	pos = line.find('V')+1;
	length = line.find('\n',pos)-pos;
	fieldType = atoi(line.substr(pos,length).c_str());

	if(staticFlag){
		/* read access to class field */
		if(offsetFlag){
			/* when fieldoffset is given */ 
		}
		else{
			/* when fieldIndex is given */ 
		}

	}
	else{
		/* read access to object field */
		if(offsetFlag){
			/* when fieldoffset is given */ 
		}
		else{
			/* when fieldoffset is given */ 
		}
	}

}


void Simulator::printStats(){
	myMemManager->printStats();
}

Simulator::~Simulator() {

}

}
