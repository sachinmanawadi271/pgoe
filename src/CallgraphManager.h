#ifndef CALLGRAPHMANAGER_H
#define CALLGRAPHMANAGER_H

#include <string>
#include <queue>
#include <numeric>	// for std::accumulate

#include <map>

#include "CgNode.h"
#include "Callgraph.h"
#include "EstimatorPhase.h"

#define PRINT_DOT_AFTER_EVERY_PHASE true
#define DUMP_INSTRUMENTED_NAMES true
#define DUMP_UNWOUND_NAMES true

class CallgraphManager {

public:
	CallgraphManager(Config* config);

	void putEdge(std::string parentName, std::string parentFilename, int parentLine,
			std::string childName, unsigned long long numberOfCalls, double timeInSeconds);

	void putNumberOfStatements(std::string name, int numberOfStatements);
	void putNumberOfSamples(std::string name, unsigned long long  numberOfSamples);
	CgNodePtr findOrCreateNode(std::string name, double timeInSeconds = 0.0);

	void registerEstimatorPhase(EstimatorPhase* phase, bool noReport = false);

	void thatOneLargeMethod();	// TODO RN: rename

	// Delegates to the underlying graph
	CgNodePtrSet::iterator begin(){return graph.begin();};
	CgNodePtrSet::iterator end(){return graph.end();};
	size_t size(){return graph.size();};

	void printDOT(std::string prefix);
private:
	// this is a legacy structure used to parse the call graph
	std::map<std::string, CgNodePtr> graphMapping;
	// this set represents the call graph during the actual computation
	Callgraph graph;
	Config* config;

	// estimator phases run in a defined order
	std::queue<EstimatorPhase*> phases;

	void putEdge(std::string parentName, std::string childName);

	void finalizeGraph();

	void dumpInstrumentedNames(CgReport report);
	void dumpUnwoundNames(CgReport report);
};


#endif
