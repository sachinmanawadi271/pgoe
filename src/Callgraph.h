#ifndef CALLGRAPH_H
#define CALLGRAPH_H

#include <string>
#include <queue>
#include <numeric>	// for std::accumulate

#include <unordered_map>

#include "CgNode.h"
#include "EstimatorPhase.h"

class Callgraph {

public:
	Callgraph(int samplesPerSecond=10000);
	int putFunction(std::string parentName, std::string childName);
	int putFunction(std::string parentName, std::string parentFilename, int parentLine,
			std::string childName, unsigned long long numberOfCalls, double timeInSeconds);

	void registerEstimatorPhase(EstimatorPhase* phase);

	void thatOneLargeMethod();	// TODO RN: rename

private:
	// this is a legacy structure used to parse the call graph
	std::unordered_map<std::string, CgNodePtr> graphMapping;
	// this set represents the call graph during the actual computation
	CgNodePtrSet graph;
	// the target frequency for sampling
	const int samplesPerSecond;

	// estimator phases run in a defined order
	std::queue<EstimatorPhase*> phases;

	void optimizeGraph();
	void printDOT(std::string prefix);

	CgNodePtr findNode(std::string functionName); // Finds FIRST node including functionName
	CgNodePtr findMain();
};


#endif
