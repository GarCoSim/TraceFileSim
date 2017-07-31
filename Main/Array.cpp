/*
 * Array.cpp
 *
 */

#include <stdio.h>
#include "Array.hpp"

namespace traceFileSimulator{

Array::Array(int id_, void *address_, size_t size_, int numberOfPointers_, char *className_){
	id = id_;
	address = address_;
	size = size_;
	numberOfPointers = numberOfPointers_;
	className = className_;
}
}
