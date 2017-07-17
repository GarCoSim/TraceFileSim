#ifndef REGION_HPP_
#define REGION_HPP_

#include <vector>
#include <set>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <math.h>
#include "Object.hpp"

using std::vector;
namespace traceFileSimulator {

/** Class for containing meta data about a region in
 * a region based heap.
 */
class Region {
public:
	virtual ~Region();	
	Region(void *address, size_t size, unsigned char* heapAddress);
	
	void	setSize(size_t size);

	void*	getAddress();

	unsigned char* getHeapAddress(); // real address
	 
	void	setCurrFree(size_t free); 
	size_t	getCurrFree();

	void	incrementObjectCount(); 
	int		getNumObj(); 
	void	setNumObj(int objects);

	void	setCurrFreeAddr(void* addr); 
	void*	getCurrFreeAddr(); //virtuel address

	void	setAge(int age);
	int		getAge();

	std::set<void*> getRemset();

	void	insertObjectReference(void* address);
	void	eraseObjectReferenceWithoutCheck(void* address);

	void	setIsLeaf(bool isLeaf);
	bool	getIsLeaf();

	void reset();

	std::set<Object*> spineRemset; //list of all spines in this region
	bool isArrayletleaf;
	Object *associatedSpine; //if the region is a Arrayletleaf we need to check the associated spine if we can free this region

protected:
	size_t mySize;
	unsigned char* myHeapAddress;
	void* myAddress;
	int myAge;
	std::set<void*> myRemset; //remembered set 

	int numObj;			//how many objects in the region
	size_t currFree;	//how much free space in a region
	void *currFreeAddr;	//address of the free area in the region

	
	bool isArrayletLeaf;
	size_t ref_count;
};

}
#endif /* REGION_HPP_ */
