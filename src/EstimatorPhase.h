#ifndef ESTIMATORPHASE_H_
#define ESTIMATORPHASE_H_

#include <string>
#include <iostream>

#include <memory>
#include <queue>
#include <unordered_set>

#include <cassert>

#include "CgNode.h"
#include "CgHelper.h"


struct CgReport {
	unsigned int instrumentedMethods;
	unsigned int overallMethods;

	unsigned long long instrumentedCalls;
	unsigned long long unwindSamples;

	double instrumentationOverhead;	// nanos
	double unwindOverhead;	// nanos

	std::string phaseName;

};

class EstimatorPhase {
public:
	EstimatorPhase(std::string name);
	virtual ~EstimatorPhase() {}

	virtual void modifyGraph(CgNodePtr mainMethod) = 0;

	void generateReport();

	void setGraph(CgNodePtrSet* graph);

	struct CgReport getReport();
	virtual void printReport();

protected:
	CgNodePtrSet* graph;

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
	RemoveUnrelatedNodesEstimatorPhase();
	~RemoveUnrelatedNodesEstimatorPhase();

	void modifyGraph(CgNodePtr mainMethod);

	void printReport();
protected:
	void printAdditionalReport();
private:
	int numRemovedNodes;
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

	int numberOfConjunctions;

	struct ConjunctionDependency {
		CgNodePtrSet dependentConjunctions;
		CgNodePtrSet markerPositions;

		ConjunctionDependency(CgNodePtr dependentConjunctions, CgNodePtrSet markerPositions) {
			CgNodePtrSet nodeSet = {dependentConjunctions};
			this->dependentConjunctions = nodeSet;
			this->markerPositions = markerPositions;
		}
	};

	struct ConjunctionDependencies {
		std::vector<ConjunctionDependency> dependencies;

//		void addDependency(CgNodePtr conjunction, CgNodePtrSet markers) {
//			dependencies.push_back(ConjunctionDependency(conjunction, markers));
//		}

//		void addDependencyFor(CgNodePtr oldConjunction, CgNodePtr newConjunction, CgNodePtrSet newMarkers) {
//			for (auto dependency : dependencies) {
//				if (dependency.dependentConjunctions.find(oldConjunction) != dependency.dependentConjunctions.end()) {
//					dependency.dependentConjunctions.insert(newConjunction);
//					dependency.markerPositions.insert(newMarkers.begin(), newMarkers.end());
//					return;
//				}
//			}
//			assert(0 && "no dependency found");
//		}

		bool hasDependencyFor(CgNodePtr conjunction) {
			for (auto dependency : dependencies) {
				if (dependency.dependentConjunctions.find(conjunction) != dependency.dependentConjunctions.end()) {
					return true;
				}
			}
			return false;
		}

	};
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
