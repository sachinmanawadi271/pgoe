#ifndef CALLGRAPH_H
#define CALLGRAPH_H

#include <string>
#include <queue>
#include <numeric>	// for std::accumulate

#include "CgNode.h"
#include "EstimatorPhase.h"
#include "SanityCheckEstimatorPhase.h"
#include "MinimalSpantreeEstimatorPhase.h"

class Callgraph {

public:
	Callgraph(int samplesPerSecond=10000);
	int putFunction(std::string parentName, std::string childName);
	int putFunction(std::string parentName, std::string parentFilename, int parentLine,
			std::string childName, unsigned long long numberOfCalls, double timeInSeconds);

	CgNodePtr findNode(std::string functionName); // Finds FIRST node including functionName
	CgNodePtr findMain();

	std::vector<CgNodePtr> getNodesRequiringInstrumentation();
	int getSize();

	void updateNodeAttributes();

	void registerEstimatorPhases();
	void thatOneLargeMethod();	// TODO RN: rename

	void print();
	void printDOT(std::string prefix);

private:
	// this is a legacy structure used to parse the call graph
	std::unordered_map<std::string, CgNodePtr> graphMapping;
	// this set represents the call graph during the actual computation
	CgNodePtrSet graph;
	// the target frequency for sampling
	const int samplesPerSecond;

	// estimator phases run in a defined order
	std::queue<EstimatorPhase*> phases;
};


#endif
