#ifndef ESTIMATORPHASE_H_
#define ESTIMATORPHASE_H_

#include <string>
#include <iostream>
#include <iomanip>	//  std::setw()

#include <memory>
#include <queue>
#include <set>

#include <cassert>

#include "CgNode.h"
#include "CgHelper.h"
#include "Callgraph.h"

struct CgReport {

	CgReport() :
		instrumentedMethods(0),
		overallMethods(0),
		instrumentedCalls(0),
		unwindSamples(0),
		instrumentationOverhead(.0),
		unwindOverhead(.0),
		phaseName(std::string()),
		instrumentedNames(std::set<std::string>())
	{}

	unsigned int instrumentedMethods;
	unsigned int overallMethods;

	unsigned long long instrumentedCalls;
	unsigned long long unwindSamples;

	double instrumentationOverhead;	// nanos
	double unwindOverhead;	// nanos

	std::string phaseName;

	std::set<std::string> instrumentedNames;

};

class EstimatorPhase {
public:
	EstimatorPhase(std::string name);
	virtual ~EstimatorPhase() {}

	virtual void modifyGraph(CgNodePtr mainMethod) = 0;

	void generateReport();

	void setGraph(Callgraph* graph);

	struct CgReport getReport();
	virtual void printReport();

protected:
	Callgraph* graph;

	CgReport report;
	std::string name;

	/* print some additional information of the phase */
	virtual void printAdditionalReport() {}
};

/**
 * Remove nodes from the graph that are not connected to the main() method.
 */
class RemoveUnrelatedNodesEstimatorPhase : public EstimatorPhase {
public:
	RemoveUnrelatedNodesEstimatorPhase(bool onlyRemoveUnrelatedNodes = true, bool aggressiveReduction = false);
	~RemoveUnrelatedNodesEstimatorPhase();

	void modifyGraph(CgNodePtr mainMethod);

	void printReport();
private:
	void printAdditionalReport();
	void checkLeafNodeForRemoval(CgNodePtr node);

	int numUnconnectedRemoved;
	int numLeafsRemoved;
	int numChainsRemoved;
	int numAdvancedOptimizations;

	bool aggressiveReduction;
	bool onlyRemoveUnrelatedNodes;

	CgNodePtrSet nodesToRemove;
};

/**
 * Read out some statistics about the current call graph
 */
class GraphStatsEstimatorPhase : public EstimatorPhase {
public:
	GraphStatsEstimatorPhase();
	~GraphStatsEstimatorPhase();

	void modifyGraph(CgNodePtr mainMethod);
	void printReport();
private:
	void printAdditionalReport();
	bool hasDependencyFor(CgNodePtr conjunction) {
		for (auto dependency : dependencies) {
			if (dependency.dependentConjunctions.find(conjunction) != dependency.dependentConjunctions.end()) {
				return true;
			}
		}
		return false;
	}

private:
	struct ConjunctionDependency {
		CgNodePtrSet dependentConjunctions;
		CgNodePtrSet markerPositions;

		ConjunctionDependency(CgNodePtrSet dependentConjunctions, CgNodePtrSet markerPositions) {
			this->dependentConjunctions = dependentConjunctions;
			this->markerPositions = markerPositions;
		}
	};

private:
	int numCyclesDetected;

	int numberOfConjunctions;
	std::vector<ConjunctionDependency> dependencies;
	std::set<CgNodePtr> allValidMarkerPositions;
};

class DiamondPatternSolverEstimatorPhase : public EstimatorPhase {
public:
	DiamondPatternSolverEstimatorPhase();
	~DiamondPatternSolverEstimatorPhase();

	void modifyGraph(CgNodePtr mainMethod);
	void printReport();
private:
	int numDiamonds;
	int numUniqueConjunction;	// all potential marker positions are necessary
	int numOperableConjunctions;	// there is one marker position per path

	void printAdditionalReport();
};

/**
 * Instrument all call conjunctions.
 */
class InstrumentEstimatorPhase : public EstimatorPhase {
public:
	InstrumentEstimatorPhase();
	~InstrumentEstimatorPhase();

	void modifyGraph(CgNodePtr mainMethod);
};

/**
 * Move Instrumentation hooks upwards a call chain if overhead decreases.
 */
class MoveInstrumentationUpwardsEstimatorPhase : public EstimatorPhase {
public:
	MoveInstrumentationUpwardsEstimatorPhase();
	~MoveInstrumentationUpwardsEstimatorPhase();

	void modifyGraph(CgNodePtr mainMethod);
protected:
	void printAdditionalReport();
private:
	int movedInstrumentations;
};

/**
 * For every conjunction, delete instrumentation in the most expensive parent node.
 */
class DeleteOneInstrumentationEstimatorPhase : public EstimatorPhase {
public:
	DeleteOneInstrumentationEstimatorPhase();
	~DeleteOneInstrumentationEstimatorPhase();

	void modifyGraph(CgNodePtr mainMethod);
protected:
	void printAdditionalReport();
private:
	int deletedInstrumentationMarkers;
};

/**
 * Instrument all conjunction nodes.
 * This assumes that the instrumentation hook contains the call site.
 */
class ConjunctionEstimatorPhase : public EstimatorPhase {
public:
	ConjunctionEstimatorPhase(bool instrumentOnlyConjunctions = false);
	~ConjunctionEstimatorPhase();

	void modifyGraph(CgNodePtr mainMethod);
private:
	bool instrumentOnlyConjunctions;
};

/**
 * Local Unwind from all leaf nodes.
 * Remove redundant instrumentation.
 * TODO RN: this phase can not deal with the advanced instrumentation from NodeBasedOptimum
 */
class UnwindEstimatorPhase : public EstimatorPhase {
public:
	UnwindEstimatorPhase();
	~UnwindEstimatorPhase();

	void modifyGraph(CgNodePtr mainMethod);
protected:
	void printAdditionalReport();
private:
	int unwoundNodes;
	int unwindCandidates;
};

class ResetEstimatorPhase : public EstimatorPhase {
public:
	ResetEstimatorPhase();
	~ResetEstimatorPhase();

	void modifyGraph(CgNodePtr mainMethod);
protected:
	void printAdditionalReport();
};

#endif
