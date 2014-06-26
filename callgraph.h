#ifndef CALLGRAPH_H
#define CALLGRAPH_H

#include <map>
#include <string>
#include <queue>

#include "cgNode.h"
#include "estimatorphase.h"

class Callgraph {

public:
	Callgraph(int samplesPerSecond=10000);
	int putFunction(std::string parentName, std::string childName);
	int putFunction(std::string parentName, std::string parentFilename, int parentLine,
			std::string childName, unsigned long long numberOfCalls, double timeInSeconds);

	std::shared_ptr<CgNode> findNode(std::string functionName); // Finds FIRST node including functionName
	std::shared_ptr<CgNode> findMain();

	std::vector<std::shared_ptr<CgNode> > getNodesRequiringInstrumentation();
	int getSize();

	void updateNodeAttributes();

	void thatOneLargeMethod();	// TODO RN: rename

	void print();
	void printDOT(std::string prefix);

private:
	std::map<std::string, std::shared_ptr<CgNode> > graph;

	const int samplesPerSecond;

	std::queue<EstimatorPhase*> phases;
};


#endif
