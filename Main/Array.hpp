/*
 * Array.hpp
 *
 */


#ifndef ARRAY_HPP_
#define ARRAY_HPP_

namespace traceFileSimulator{

class Array {
	public:
	int id;
	void *address;
	size_t size;
	int numberOfPointers;
	char *className;

	Array(int id, void *address, size_t size, int numberOfPointers, char *className);

};

}
#endif /* ARRAY_HPP_ */
