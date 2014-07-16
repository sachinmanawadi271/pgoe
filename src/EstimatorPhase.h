#ifndef ESTIMATORPHASE_H_
#define ESTIMATORPHASE_H_

#include <string>
#include <iostream>

#include <memory>
#include <map>

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
	EstimatorPhase(std::map<std::string, CgNodePtr >* graph, std::string name);
	virtual ~EstimatorPhase() {}

	virtual void modifyGraph(CgNodePtr mainMethod) = 0;

	void generateReport();

	struct CgReport getReport();
	virtual void printReport();

protected:
	std::map<std::string, CgNodePtr >* graph;

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
	RemoveUnrelatedNodesEstimatorPhase(std::map<std::string, CgNodePtr >* graph);
	~RemoveUnrelatedNodesEstimatorPhase();

	void modifyGraph(CgNodePtr mainMethod);

	void printReport();
protected:
	void printAdditionalReport();
private:
	int numRemovedNodes;
};

#include <queue>
#include <set>

/**
 * Instrument all call conjunctions.
 */
class InstrumentEstimatorPhase : public EstimatorPhase {
public:
	InstrumentEstimatorPhase(std::map<std::string, CgNodePtr>* graph);
	~InstrumentEstimatorPhase();

	void modifyGraph(CgNodePtr mainMethod);
};

/**
 * Move Instrumentation hooks upwards a call chain if overhead decreases.
 */
class MoveInstrumentationUpwardsEstimatorPhase : public EstimatorPhase {
public:
	MoveInstrumentationUpwardsEstimatorPhase(std::map<std::string, CgNodePtr>* graph);
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
	DeleteOneInstrumentationEstimatorPhase(std::map<std::string, CgNodePtr>* graph);
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
 */
class UnwindEstimatorPhase : public EstimatorPhase {
public:
	UnwindEstimatorPhase(std::map<std::string, CgNodePtr>* graph);
	~UnwindEstimatorPhase();

	void modifyGraph(CgNodePtr mainMethod);
protected:
	void printAdditionalReport();
private:
	int unwoundNodes;
	int unwindCandidates;
};

#endif
