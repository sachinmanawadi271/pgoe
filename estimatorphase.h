#ifndef ESTIMATORPHASE_H_
#define ESTIMATORPHASE_H_

#include <string>
#include <iostream>

#include <memory>
#include <map>

#include "cgNode.h"

struct CgReport {
	unsigned long long instrumentedCalls;
	unsigned long long unwindSamples;

	double instrumentationOverhead;	// nanos
	double unwindOverhead;	// nanos

	std::string phaseName;

	void print() {
		std::cout << "Report Phase ==" << phaseName << "==" << std::endl;
		std::cout << "\tinstrumentedCalls: " << instrumentedCalls
				<< " | instrumentationOverhead: " << instrumentationOverhead << " ns" << std::endl
				<< "\tunwindSamples: " << unwindSamples
				<< " | undwindOverhead: " << unwindOverhead << " ns" << std::endl
				<< "\toverallOverhead: " << (instrumentationOverhead+unwindOverhead) << " ns"<< std::endl;
	}
};

class EstimatorPhase {
public:
	EstimatorPhase(std::map<std::string, std::shared_ptr<CgNode> >* graph, std::string name);
	virtual ~EstimatorPhase() {}

	virtual void modifyGraph(std::shared_ptr<CgNode> mainMethod) = 0;

	void generateReport();

	struct CgReport getReport();

protected:
	std::map<std::string, std::shared_ptr<CgNode> >* graph;

	CgReport report;

	std::string name;

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
