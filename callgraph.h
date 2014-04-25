#include <map>
#include <string>
#include <queue>

#include "cgNode.h"

class Callgraph {

public:
	Callgraph();
	int putFunction(std::string fullQualifiedNameCaller, std::string fullQualifiedNameCallee);
	int putFunction(std::string fullQualifiedNameCaller, std::string filenameCaller, int lineCaller, std::string fullQualifiedNameCallee, int calls);


	void print();
	void printDOT();

	std::shared_ptr<CgNode> findNode(std::string functionName); // Finds FIRST node including functionName
	std::shared_ptr<CgNode> findMain();

	std::vector<std::shared_ptr<CgNode> > getNodesToMark();
	int getSize();



private:
	std::map<std::string, std::shared_ptr<CgNode> > graph;
};
