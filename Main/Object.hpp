/*
 * Object.hpp
 *
 *  Created on: Jul 30, 2013
 *      Author: GarCoSim
 */

#ifndef OBJECT_HPP_
#define OBJECT_HPP_

#include <vector>

using std::vector;
namespace traceFileSimulator {

class Object {
public:
	Object(int id, int payloadSize, int maxPointers, int address);
	void setArgs(int id, int payloadSize, int maxPointers);
	virtual ~Object();
	int 	getAddress();
	void 	updateAddress(int newAddress);
	int 	getID();
	int 	getPayloadSize();
	int	 	getPointerCount();
	int 	getPointersMax();
	Object* getReferenceTo(int pointerNumber);
	int 	setPointer(int pointerNumber, Object* target);

	int 	getIsAlive();
	void 	setIsAlive(int value);
	int 	getVisited();
	void	setVisited(int value);

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

	void setFreed(int freed) {
		this->freed = freed;
	}

private:
	int 	myId;
	int freed;

	/*The actual object I am storing information about
	 * (not interesting for our purpose, it only has a size) */
	int  	myPayloadSize;

	/*How many objects am I pointing at? How many am I allowed to point at?*/
	int 	myPointersMax;
	int 	myPointersCurrent;

	//the starting address of the object on the virtual heap
	int myAddress;

	/*the list of objects I am pointing at*/
	Object** pointers;

	//garbage collector stuff
	//TODO those two are basically the same. one could be removed
	int isVisited;
	int isAlive;

	//genCon
	int myAge;
    int	myGeneration;

};

} 
#endif /* OBJECT_HPP_ */
