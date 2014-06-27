#ifndef ESTIMATORPHASE_H_
#define ESTIMATORPHASE_H_

#include <string>
#include <iostream>

#include <memory>
#include <map>

#include "CgNode.h"
#include "CgHelper.h"

struct CgReport {
	unsigned long long instrumentedCalls;
	unsigned long long unwindSamples;

	double instrumentationOverhead;	// nanos
	double unwindOverhead;	// nanos

	std::string phaseName;

};

class EstimatorPhase {
public:
	EstimatorPhase(std::map<std::string, std::shared_ptr<CgNode> >* graph, std::string name);
	virtual ~EstimatorPhase() {}

	virtual void modifyGraph(std::shared_ptr<CgNode> mainMethod) = 0;

	void generateReport();

	struct CgReport getReport();
	virtual void printReport();

protected:
	std::map<std::string, std::shared_ptr<CgNode> >* graph;

	CgReport report;
	std::string name;

	/* print some additional information of the phase */
	virtual void printAdditionalReport() {}
};

#include <queue>
#include <set>

/**
 * Instrument call conjunctions
 */
class InstrumentEstimatorPhase : public EstimatorPhase {
public:
	InstrumentEstimatorPhase(std::map<std::string, std::shared_ptr<CgNode> >* graph);
	~InstrumentEstimatorPhase();

	void modifyGraph(std::shared_ptr<CgNode> mainMethod);
};

/**
 * While possible - Move instrumentation hooks upwards along a call chain
 */
class MoveInstrumentationUpwardsEstimatorPhase : public EstimatorPhase {
public:
	MoveInstrumentationUpwardsEstimatorPhase(std::map<std::string, std::shared_ptr<CgNode> >* graph);
	~MoveInstrumentationUpwardsEstimatorPhase();

	void modifyGraph(std::shared_ptr<CgNode> mainMethod);
};

/**
 * Select all leaves for unwind.
 * Remove redundant instrumentation.
 */
class UnwindEstimatorPhase : public EstimatorPhase {
public:
	UnwindEstimatorPhase(std::map<std::string, std::shared_ptr<CgNode> >* graph);
	~UnwindEstimatorPhase();

	void modifyGraph(std::shared_ptr<CgNode> mainMethod);
};

#endif
