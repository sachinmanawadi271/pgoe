
#include "estimatorphase.h"

EstimatorPhase::EstimatorPhase(std::map<std::string, std::shared_ptr<CgNode> >* graph, std::string name) :
graph(graph),
report({0}),	// this hopefully initializes all members to 0
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
	//XXX
	std::cout << "xxx" << report.unwindSamples << std::endl;
}

InstrumentEstimatorPhase::~InstrumentEstimatorPhase() {
}

void InstrumentEstimatorPhase::modifyGraph(std::shared_ptr<CgNode> mainMethod) {
	std::queue<std::shared_ptr<CgNode> > workQueue;
	std::vector<CgNode*> done;

	workQueue.push(mainMethod);
	while (!workQueue.empty()) {
#if DEBUG > 1
		std::cerr << "Queue Size: " << workQueue.size() << std::endl;
#endif
		auto node = workQueue.front();
		done.push_back(node.get());
		workQueue.pop();
		if (node == NULL) {
			std::cerr << "node was NULL" << std::endl;
		}
		if (node->getParentNodes().size() > 1) {
#if DEBUG > 1
			std::cout << "For node: " << node->getFunctionName() << " callers.size() = " << node->getParentNodes().size() << std::endl;
#endif
			for (auto nodeToInsert : node->getParentNodes()) {
				nodeToInsert->setState(CgNodeState::INSTRUMENT);
			}

		}
		for (auto n : node->getChildNodes()) {
			bool insert = true;
			for (auto refNode : done) {
				if (refNode == n.get()) {
					insert = false;
				}
			}
			if (insert) {
				workQueue.push(n);
			}
		}
	}
}
