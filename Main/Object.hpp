/*
 * Object.hpp
 *
 *  Created on: Jul 30, 2013
 *      Author: GarCoSim
 */

#ifndef OBJECT_HPP_
#define OBJECT_HPP_

#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> //added by Tristan; for memset in setArgs()

using std::vector;
namespace traceFileSimulator {

class Object;

typedef struct RawObject {
	void *classPool;
	Object *associatedObject;
	RawObject *pointers[];
} RawObject;

class Object {
public:
	Object(int id, void *address, int size, int numberOfPointers, char *className);
	void setArgs(int id, int payloadSize, int maxPointers, char *className);
	virtual ~Object();
	size_t 	getAddress();
	void 	updateAddress(size_t newAddress);
	int 	getID();
	int 	getPayloadSize();
	int 	getHeapSize();
	int 	getPointersMax();
	Object* getReferenceTo(int pointerNumber);
	int 	setPointer(int pointerNumber, Object* target);
	void *getRawPointerAddress(int pointerNumber);
	void setRawPointerAddress(int pointerNumber, void *address);

	bool 	getVisited();
	void	setVisited(bool value);

	int getAge() const {
		return myAge;
	}

	void setAge(int age) {
		myAge = age;
	}
	void setGeneration(int generation);
	int  getGeneration();

	int getFreed() const {
		return freed;
	}

	const char *getClassName() {
		return myName;
	}

	void setFreed(int freed) {
		this->freed = freed;
	}

	bool isForwarded() {
		return forwarded;
	}

	void setForwarded(bool value) {
		forwarded = value;
	}

private:
	RawObject *rawObject;
	int 	myId;
	int freed;

	int  	mySize;

	// How many pointer slots do I have?
	int 	myPointersMax;

	//garbage collector stuff
	bool isVisited;

	//genCon
	int myAge;
    int	myGeneration;

    char *myName;
    bool forwarded;

};

} 
#endif /* OBJECT_HPP_ */
