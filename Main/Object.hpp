/*
 * Object.hpp
 *
 *  Created on: Jul 30, 2013
 *      Author: GarCoSim
 */

#ifndef OBJECT_HPP_
#define OBJECT_HPP_

#include "../defines.hpp"
#include <vector>
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
	allocationTypeEnum allocationType;
	Object(int id, void *address, size_t size, size_t numberOfPointers, char *className, allocationTypeEnum _allocationType);
	// modified by mazder, constructor overloading with thread id
	//Object(int tid, int id, void *address, size_t size, int maxPointers, char *className);
	void setArgs(int id, size_t payloadSize, int maxPointers, char *className);
	virtual ~Object();
	void*	getAddress();
	void 	updateAddress(void *newAddress);
	int 	getID();
	size_t 	getHeapSize();
	size_t 	getPointersMax();
	Object* getReferenceTo(size_t pointerNumber);
	int 	setPointer(size_t pointerNumber, Object* target);
	void*	getRawPointerAddress(size_t pointerNumber);
	void	setRawPointerAddress(size_t pointerNumber, void *address);
	void 	setForwardedPointer(void* address);
	void*	getForwardedPointer();

	bool 	getVisited();
	void	setVisited(bool value);

	int getDepth();
	void setDepth(int d);

	int getAge() const {
		return myAge;
	}

	void setAge(int age) {
		myAge = age;
	}
	void setGeneration(int generation);
	int getGeneration();

	bool isForwarded() {
		return forwarded;
	}

	void setForwarded(bool value) {
		forwarded = value;
	}

	size_t getReferenceCount();
	void increaseReferenceCount();
	void decreaseReferenceCount();

	int getColor();
	void setColor(int color);

	// The following three fields are added to:
	// do escape analysis
	// find object longevity
	// see the thread realtionships
	// calculate life time
	//int myTid;
	//bool escaped;
	//unsigned long born;

private:
	RawObject *rawObject;
	int myId;
	int freed;

	size_t mySize;

	// How many pointer slots do I have?
	size_t myPointersMax;

	//garbage collector stuff
	bool isVisited;
	int depth;

	//genCon
	int myAge;
	int myGeneration;

	char *myName;
	bool forwarded;

	void *forwardedPointer;

	size_t referenceCount;
	int color;

};

}
#endif /* OBJECT_HPP_ */
