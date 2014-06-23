
#include "estimatorphase.h"

EstimatorPhase::EstimatorPhase(std::map<std::string, std::shared_ptr<CgNode> >* graph, std::string name) :
graph(graph),
name(name)
{
}

void EstimatorPhase::generateReport() {

	// TODO refactor these somewhere outside
	const unsigned int nanosPerInstrumentedCall = 4;
	const unsigned int nanosPerUnwindSample = 4000;

	for(auto pair : (*graph) ) {
		auto node = pair.second;

		if(node->needsInstrumentation()) {
			report.instrumentedCalls += node->getNumberOfCalls();
			report.instrumentationOverhead += report.instrumentedCalls * nanosPerInstrumentedCall;
		}
		if(node->needsUnwind()) {
			report.unwindSamples += node->getExpectedNumberOfSamples();
			report.unwindOverhead += report.unwindSamples * nanosPerUnwindSample;
		}

	}
	report.phaseName = name;
}

CgReport EstimatorPhase::getReport() {
	return this->report;
}



InstrumentEstimatorPhase::InstrumentEstimatorPhase(std::map<std::string, std::shared_ptr<CgNode> >* graph) :
EstimatorPhase(graph, "Instrument") {
}

InstrumentEstimatorPhase::~InstrumentEstimatorPhase() {
}

void InstrumentEstimatorPhase::modifyGraph() {
	// TODO instrument
}
