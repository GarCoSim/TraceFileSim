/*
 * Simulator.cpp
 *
 *  Created on: Sep 3, 2013
 *      Author: GarCoSim
 */

#include "Simulator.hpp"
#include "MemoryManager.hpp"
#include <string>
#include <stdlib.h>
#include <sstream>

using namespace std;

extern TRACE_FILE_LINE_SIZE gLineInTrace;
extern int gAllocations;
extern int forceAGCAfterEveryStep;

extern FILE* zombieFile;
extern int lockNumber;
extern int catchZombies;
extern int lockingStats;

namespace traceFileSimulator {

void (Simulator::*operationReference)(TraceFileLine) = NULL;

/** Creates a Simulator. Simulator creates a memory manager from some of it's parameters and opens the file at it's
 * traceFilePath. This is used at the entry point to step through the trace file line by line
 * and evaluate it's success.
 *
 * @param traceFilePath Path to the file to be parsed.
 * @param heapSize Initial Size of the heap
 * @param maxHeapSize Maximum size of the heap. Attempting to allocate data requiring more space than this should throw an error
 * @param highWatermark Maximum ratio of used space before automatically calling collector
 * @param garbageCollector The int cast of the collector enum for the chosen collector
 * @param traversal The int cast of the traversal enum for the chosen traversal method
 * @param allocator The int cast of the allocator enum for the chosen allocator
 * @param writebarrier The int cast of the writebarrier enum for the chosen write barrier
 * @param finalGC Int used to flag if a final GC needs to be called or not
 */
Simulator::Simulator(char* traceFilePath, size_t heapSize, size_t maxHeapSize, int highWatermark, int garbageCollector, int traversal, int allocator, int writebarrier, int finalGC) {
	myLastStepWorked = 1;
	myFinalGC = finalGC;
	myTraceFile.open(traceFilePath);
	if(!myTraceFile.good()){
		fprintf(stderr, "File open failed.\n");
		exit(1);
	}

	myMemManager = new MemoryManager(heapSize, maxHeapSize, highWatermark, garbageCollector, traversal, allocator, writebarrier);

	if (!myMemManager->loadClassTable((string)traceFilePath)){
		fprintf(stdout, "No class table found\n");
		// even thougth -cls 1 paramenters passed in main function
	}

	if (allocator == (int)regionBased) {
		operationReference = &Simulator::regionReferenceOperation;
	}
	else {
		operationReference = &Simulator::referenceOperation;
	}

	counter = 0;
	start = clock();
	seconds = 0;

	amountAllocatedObjects = 0;
	lockNumber = 0;

	lockingCounter.resize(10);
	for (unsigned int i = 0; i < lockingCounter.size(); i++) {
		lockingCounter[i] = 0;
	}
	lockingCounter[0] = 1; //Tracefile starts unlocked
	lockedLines = 0;
	unlockedLines = 0;
	lastLockLine = 0;
}

std::vector<int> Simulator::getLockingCounter(){
	return lockingCounter;
}

int Simulator::getLockedLines(){
	return lockedLines;
}

int Simulator::getUnlockedLines(){
	return unlockedLines;
}

void Simulator::updateUnlockedLines() {
	unlockedLines = unlockedLines + (gLineInTrace - lastLockLine) - 1;
}

/** Resets the contents of the line structure to unset optionals.
 *
 * @param line
 */
void Simulator::initializeTraceFileLine(TraceFileLine *line) {
    /* We use optionals for all
     * these arguments since not
     * all of them have good
     * default values. Given an
     * Optional o:
     *
     * Where good defaults make sense use:
     *      o.getOr(default);
     *
     * Where good defaults don't make sense
     * and nonexistence is a fatal error use:
     *      o.getValue();
     *
     * Where good defaults don't make sense
     * and nonexistence isn't an error, pass
     * the optional as a parameter and use the
     * following in the function:
     *      if(o.isSet()){
     *          T v = o.getValue();
     *          // write code logic
     *      }
     */
	line->type = Optional<char>();
	line->classID = Optional<int>();
	line->fieldIndex = Optional<int>();
	line->fieldOffset = Optional<int>();
	line->fieldSize = Optional<int>();
	line->fieldType = Optional<int>();
	line->objectID = Optional<int>();
	line->parentID = Optional<int>();
	line->parentSlot = Optional<size_t>();
	line->maxPointers = Optional<size_t>();
	line->size = Optional<size_t>();
	line->threadID = Optional<int>();
	line->lockStatus = -1;
}

/** Modifies the line to contain the correct contents based on the next line in the trace file.
 *
 * @param line A structure representing the current line of the trace file
 */
void Simulator::getNextLine(TraceFileLine *line){
	if(myTraceFile.eof()) {
		myLastStepWorked = false;
		return;
	}

	string currentLineString = "";
	getline(myTraceFile, currentLineString);
	char *currentLine = (char*) currentLineString.c_str();
	gLineInTrace++;

	if (!line)
		return; // caller didn't care about the parsed attributes

	initializeTraceFileLine(line);
	line->type.setValue(currentLine[0]);
	if(currentLine[0] == '%'){
		if(gLineInTrace==1){
			fprintf(stdout, "%s\n", currentLineString.c_str());
		}
		return;
	}

	string buffer = "";
	char attributeID;
	int i = 1;
	while (currentLine[i] != '\0') {
		while (currentLine[i] == ' ') // burn extra whitespace between attributes
			i++;

		attributeID = currentLine[i++];
		buffer.clear();
		while (currentLine[i]!=' ' && currentLine[i]!='\0')
			buffer.append(1, currentLine[i++]);

		std::stringstream sstream(buffer);

		switch (attributeID) {
			case ('C'):
				int cID;
				sstream >> cID;
				line->classID.setValue(cID);
				break;
			case ('I'):
				int fieldIndex;
				sstream >> fieldIndex;
				line->fieldIndex.setValue(fieldIndex);
				break;
			case ('F'):
				int fieldOffset;
				sstream >> fieldOffset;
				line->fieldOffset.setValue(fieldOffset);
				break;
			case ('S'):
				size_t size;
				sstream >> size;
				line->size.setValue(size);
				break;
			case ('V'):
				int type;
				sstream >> type;
				line->fieldType.setValue(type);
				break;
			case ('O'):
				int oID;
				sstream >> oID;
				line->objectID.setValue(oID);
				break;
			case ('P'):
				int pID;
				sstream >> pID;
				line->parentID.setValue(pID);
				break;
			case ('#'):
				size_t slot;
				sstream >> slot;
				line->parentSlot.setValue(slot);
				break;
			case ('N'):
				size_t pointers;
				sstream >> pointers;
				line->maxPointers.setValue(pointers);
				break;
			case ('T'):
				int tID;
				sstream >> tID;
				line->threadID.setValue(tID);
				break;
			case ('L'):
				line->lockStatus = val; break;
			default:
				fprintf(stderr, "Invalid form in getNextLine, execution should never reach this line. Line: %i . Attribute: %c\n", gLineInTrace, attributeID);
				break;
		}
	}
}

/** Delegate method to call the MemoryManger::lastStats()
 *
 */
void Simulator::lastStats() {
	myMemManager->lastStats();
}

/** Creates a TraceFileLine and calls Simulator::getNextLine(TraceFileLine) on it.
 *  Uses this line to call the appropriate function for each possible trace file line.
 * @return always returns 0
 */
int Simulator::doNextStep(){
	TraceFileLine line;
	getNextLine(&line);
	if (ONE_SECOND_PASSED) {
		start = clock();
		seconds++;
		printf("[%3ds] Line in tracefile: " TRACE_FILE_LINE_FORMAT "\n", seconds, gLineInTrace);
	}
	if(myLastStepWorked){
		//if content exists, advice the MM(memory manager) to execute
		switch(line.type.getValue()) {
			case 'w':
				(*this.*operationReference)(line);
				break;
			case 'a':
				allocateToRootset(line);
			    amountAllocatedObjects++;
				break;
			case '+':
				addToRoot(line);
				break;
			case '-':
				deleteRoot(line);
				break;
			case 'c':
				referenceOperationClassField(line);
				break;
			case 'r': // for now we ignore the class option
				// currently doesn't do anything
				readOperation(line);
				break;
			case 's': // for now we ignore the class option
				storeOperation(line);
				break;
			case 'x':
				lockOperation(line);
				break;
			default:
				//gLineInTrace++;
			break;
		}

	}
	else {
		//Last line in traceFile reached
		if (myFinalGC) {
			myMemManager->forceGC();
		}

		//Last GC executed. Print all zombies:
		if (catchZombies) {
			std::map<int, int> zombies = myMemManager->getZombies();
			std::map<int, int> allocateLines = myMemManager->getObjectsAllocateLines();
			
			std::map<int, int>::iterator it;
			fprintf(zombieFile, "Amount of zombies: %lu\n", zombies.size());
			fprintf(zombieFile, "Amount of allocated objects: %u\n", amountAllocatedObjects);
			fprintf(zombieFile, "%.2f Percent of Objects are zombies.\n", (float)((zombies.size()*100)/(float)amountAllocatedObjects));

			int allocateLine;
			int zombieMinusAllocate;
			for (it = zombies.begin(); it != zombies.end(); ++it) {	
				allocateLine = allocateLines.find(it->first)->second;
				zombieMinusAllocate = it->second - allocateLine;
				fprintf(zombieFile, "O%i Lifetime: %i Allocated: %i Zombie: %i\n", it->first, zombieMinusAllocate, allocateLine, it->second);
			}
		}
	}
	// commented by mazder
	if (forceAGCAfterEveryStep)
		myMemManager->forceGC();

	/*This line calls a garbage collec after each line. Usually useful
	 * in order to analyse the actual heap use of the file.*/
	//myMemManager->evalCollect();

	/*print stats to log file after each line
	 *.. usually used for result collection and comparison*/
	//myMemManager->printStats();


	return 0;
}

/** Used to determine if the last Simulator::doNextStep() concluded successfully
 *
 * @return 1 if last step worked, else 0
 */
int Simulator::lastStepWorked(){
	if(myLastStepWorked == 1){
		return 1;
	}
	return 0;
}

/** Delegate method to call the MemoryManger::allocateObjectToRootset(int, int, size_t, int, int)
 *
 */
void Simulator::allocateToRootset(TraceFileLine line){
	myMemManager->allocateObjectToRootset(
			line.threadID.getValue(),
			line.objectID.getValue(),
			line.size.getValue(),
			line.maxPointers.getValue(),
            line.classID.getOr(-1));
}

/** Delegate method to call the MemoryManger::requestRootDelete(int, int)
 *
 */
void Simulator::deleteRoot(TraceFileLine line){
	myMemManager->requestRootDelete(line.threadID.getValue(), line.objectID.getValue());
}

/** Delegate method to call the MemoryManger::requestRootAdd(int, int)
 *
 */
void Simulator::addToRoot(TraceFileLine line){
	myMemManager->requestRootAdd(line.threadID.getValue(), line.objectID.getValue());
}

/** Delegate method to call the MemoryManger::setPointer(int, int, int, int)
 *
 */
void Simulator::referenceOperation(TraceFileLine line){
	myMemManager->setPointer(
			line.threadID.getValue(),
			line.parentID.getValue(),
			line.parentSlot.getValue(),
			line.objectID.getValue());
}

/** Delegate method to call the MemoryManger::regionSetPointer(int, int, int, int)
 *
 */
void Simulator::regionReferenceOperation(TraceFileLine line){
	myMemManager->regionSetPointer(
			line.threadID.getValue(),
			line.parentID.getValue(),
			line.parentSlot.getValue(),
			line.objectID.getValue());
}

/** Delegate method to call the MemoryManger::setStaticPointer(int, int, int)
 *
 */
void Simulator::referenceOperationClassField(TraceFileLine line){
	myMemManager->setStaticPointer(
			line.classID.getValue(),
			line.fieldOffset.getValue(),
			line.objectID.getValue());
}

void Simulator::lockOperation(TraceFileLine line){

	if (lockingStats) {
		if (lockNumber == 0) 
			unlockedLines = unlockedLines + (gLineInTrace - lastLockLine) - 1;
		else 
			lockedLines = lockedLines + (gLineInTrace - lastLockLine) - 1;
	}

	if (line.lockStatus == 1) {
		lockNumber = lockNumber + 1;
	}
	else if (line.lockStatus == 0) {
		lockNumber = lockNumber - 1;
	}
	else {
		fprintf(stderr, "Invalid locking value\n");
	}

	if (lockNumber < 0) {
		fprintf(stderr, "Negative lockNumber at line: %i\n", gLineInTrace);
	}

	if (lockingStats) {
		lockingCounter[lockNumber] = lockingCounter[lockNumber] + 1;
		lastLockLine = gLineInTrace;
	}

	if (lockNumber == 0) {
		myMemManager->checkForDeadObjects();
	}
}

/** Used for read operations. Not implemented as of the below date
 *
 * @date May, 2017
 * @param line
 */
void Simulator::readOperation(TraceFileLine line){
	//Check if object is already a zombie
	if (catchZombies && line.objectID != -1)
		myMemManager->readObject(line.objectID);

	bool staticFlag = false; 	 // To decide reading is either from a class field ( static field) or an object field    
	bool offsetFlag = false; 	 // to decide either offest or index is given

	//fprintf(stderr, "Reading %i\n", line.objectID);

	if (line.classID.isSet())
		staticFlag = true;
	if (line.fieldOffset.isSet())
		offsetFlag = true;

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

/**  Store static primitive field in class and primitive field in an object
 *
 * @param line TraceFileLine structure containing the information from the line in the trace file.
 */
void Simulator::storeOperation(TraceFileLine line){
	//Check if object is already a zombie
	if (catchZombies && line.objectID != -1)
		myMemManager->readObject(line.objectID);

	bool staticFlag = false; 	 // To decide reading is either from a class field ( static field) or an object field
	bool offsetFlag = false; 	 // to decide either offest or index is given

	if (line.classID.isSet())
		staticFlag = true;
	if (line.fieldOffset.isSet())
		offsetFlag = true;

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

/** Delegate method to call the MemoryManger::printStats()
 *
 */
void Simulator::printStats(){
	myMemManager->printStats();
}

/** Deletes the created memory manager on destruction.
 *
 */
Simulator::~Simulator() {
	delete(myMemManager);
}

}
