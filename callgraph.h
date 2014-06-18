#include <map>
#include <string>
#include <queue>

#include "cgNode.h"

class Callgraph {

public:
	Callgraph();
	int putFunction(std::string parentName, std::string childName);
	int putFunction(std::string parentName, std::string parentFilename, int parentLine, std::string childName, int numberOfCalls);


	void print();
	void printDOT(std::string prefix);

	std::shared_ptr<CgNode> findNode(std::string functionName); // Finds FIRST node including functionName
	std::shared_ptr<CgNode> findMain();

	std::vector<std::shared_ptr<CgNode> > getNodesToMark();
	int getSize();

	int markNodes();
	int moveHooksUpwards();

	void markUniqueParents();

private:
	std::map<std::string, std::shared_ptr<CgNode> > graph;
};
