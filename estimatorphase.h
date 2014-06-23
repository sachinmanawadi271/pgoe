#ifndef ESTIMATORPHASE_H_
#define ESTIMATORPHASE_H_

#include <string>
#include <iostream>

#include <memory>
#include <map>

#include "cgNode.h"

struct CgReport {
	// XXX initialize these?
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
				<< " | undwindOverhead: " << unwindOverhead << " ns"<< std::endl;
	}
};

class EstimatorPhase {
public:
	EstimatorPhase(std::map<std::string, std::shared_ptr<CgNode> >* graph, std::string name);
	virtual ~EstimatorPhase() {}

	virtual void modifyGraph() = 0;

	void generateReport();

	struct CgReport getReport();

protected:
	std::map<std::string, std::shared_ptr<CgNode> >* graph;

	CgReport report;

	std::string name;

};

class InstrumentEstimatorPhase : public EstimatorPhase {
public:
	InstrumentEstimatorPhase(std::map<std::string, std::shared_ptr<CgNode> >* graph);
	~InstrumentEstimatorPhase();

	void modifyGraph();
};



#endif
