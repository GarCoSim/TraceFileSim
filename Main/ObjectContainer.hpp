#ifndef OBJECTCONTAINER_HPP_
#define OBJECTCONTAINER_HPP_

#include "Object.hpp"
#include <vector>
#include <map>

#include "Optional.cpp"
namespace traceFileSimulator {

class ObjectContainer {
public:
	ObjectContainer();
	virtual ~ObjectContainer();
	int add(Object* newObject);
	int addToRoot(Object* newObject, int thread);
	int removeFromRoot(int thread, int objectID);
	void addToGenRoot(Object* object);
	int removeFromGenRoot(Object* object);
	int removeReferenceTo(Object* object);
	void setStaticReference(int classID, int fieldOffset, int objectID);
	Object *getStaticReference(int classID, int fieldOffset);
	vector<Object*> getAllStaticReferences();
	Object* getByID(int id);
	Object* getRoot(int thread, int objectID);
	int deleteObject(Object* object, bool deleteFlag);
	int deleteObject(int objectID, bool deleteFlag);
	size_t getGenRootSize();
	Object* getGenRoot(size_t slot);
	void clearRemSet();
	vector<Object*> getRoots(size_t thread);
	vector<Object*> getLiveObjects();
//	void visualizeState(char* filename);
	size_t countElements();
	bool isAlreadyRoot(int thread, int objectID);
private:
	bool doesObjectExistInList(Object *queryObject);
	Optional<size_t>* getRemSetSlot();

	vector<std::multimap<int, Object*> > rootset;
	//vector<std::map<int, Object*> > rootset;

    std::map<int, Object*> objectMap;

	vector<Object*> remSet;
	vector<std::map<int, Object*> > classReferences;

	int rootCount;
	int remCount;
};

}
#endif /* OBJECTCONTAINER_HPP_ */
