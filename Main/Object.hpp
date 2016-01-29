/*
 * Object.hpp
 *
 *  Created on: Jul 30, 2013
 *      Author: GarCoSim
 */

#ifndef OBJECT_HPP_
#define OBJECT_HPP_

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

	Object(int id, void *address, size_t size, int numberOfPointers, char *className);
	void setArgs(int id, size_t payloadSize, int maxPointers, char *className);
	virtual ~Object();
	void*   getAddress();
	void 	updateAddress(void *newAddress);
	int 	getID();
	size_t 	getPayloadSize();
	size_t 	getHeapSize();
	int 	getPointersMax();
	Object* getReferenceTo(int pointerNumber);
	int 	setPointer(int pointerNumber, Object* target);
	void *getRawPointerAddress(int pointerNumber);
	void setRawPointerAddress(int pointerNumber, void *address);


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

	void setForwardedPointer (RawObject* target){
		rawObject = target;
	}

	RawObject* getForwardedPointer (){
		return forwardedPointer;
	}

	size_t getRegion(size_t heap, size_t regionSize); //get the region where the object belongs to

	int getReferenceCount();
	void increaseReferenceCount();
	void decreaseReferenceCount();

	int getColor();
	void setColor(int color);

private:
	RawObject *rawObject;
	int 	   myId;
	int        freed;

	size_t     mySize;

	// How many pointer slots do I have?
	int 	   myPointersMax;

	//garbage collector stuff
	bool       isVisited;
	int depth;

	//genCon
	int        myAge;
    int	       myGeneration;

    char      *myName;
    bool       forwarded;

    int referenceCount;
    int color;

    RawObject *forwardedPointer;

};

}
#endif /* OBJECT_HPP_ */
