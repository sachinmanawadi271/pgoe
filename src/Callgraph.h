#ifndef CALLGRAPH_H
#define CALLGRAPH_H

#include <string>
#include <queue>
#include <numeric>	// for std::accumulate

#include <unordered_map>

#include "CgNode.h"
#include "EstimatorPhase.h"

class CallgraphManager {

public:
	CallgraphManager(int samplesPerSecond=10000);

	void putEdge(std::string parentName, std::string parentFilename, int parentLine,
			std::string childName, unsigned long long numberOfCalls, double timeInSeconds);

	CgNodePtr findOrCreateNode(std::string name, double timeInSeconds = 0.0);

	void registerEstimatorPhase(EstimatorPhase* phase);

	void thatOneLargeMethod();	// TODO RN: rename

	// Delegates to the underlying graph
	CgNodePtrSet::iterator begin(){return graph.begin();};
	CgNodePtrSet::iterator end(){return graph.end();};
	size_t size(){return graph.size();};

	// Finds the main function in the CallGraph
	CgNodePtr findMain();

private:
	// this is a legacy structure used to parse the call graph
	std::unordered_map<std::string, CgNodePtr> graphMapping;
	// this set represents the call graph during the actual computation
	Callgraph graph;
	// the target frequency for sampling
	int samplesPerSecond;	// XXX make this const?

	// estimator phases run in a defined order
	std::queue<EstimatorPhase*> phases;

	void putEdge(std::string parentName, std::string childName);

	void finalizeGraph();
	void printDOT(std::string prefix);

	CgNodePtr findNode(std::string functionName); // Finds FIRST node including functionName
};


#endif
