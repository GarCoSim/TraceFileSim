/*
 * ThreadOwnedRegion.hpp
 *
 *  Created on: 2015-10-30
 *      Author: GarCoSim
 *
 */

#ifndef _THREADOWNEDREGION_HPP_
#define _THREADOWNEDREGION_HPP_

#include "Region.hpp"
 
namespace traceFileSimulator {

class ThreadOwnedRegion : public Region {
public:
	ThreadOwnedRegion(void *address,int size,int owner);
	virtual ~ThreadOwnedRegion();
	
	void setOwner(int owner); 
    int  getOwner();  


private:
	int   myOwner;       //assigned thread
};

extern ThreadOwnedRegion**      regions;

} 
#endif /* _THREADOWNEDREGION_HPP_ */
