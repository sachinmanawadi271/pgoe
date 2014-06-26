
#include "EstimatorPhase.h"

EstimatorPhase::EstimatorPhase(
		std::map<std::string, std::shared_ptr<CgNode> >* graph, std::string name) :

		graph(graph),
		report({0}),// this hopefully initializes all members to 0
		name(name) {
}

void EstimatorPhase::generateReport() {

	// TODO refactor these somewhere outside
	const unsigned int nanosPerInstrumentedCall = 4;
	const unsigned int nanosPerUnwindSample = 4000;

	for(auto pair : (*graph) ) {
		auto node = pair.second;

		if(node->isInstrumented()) {
			report.instrumentedCalls += node->getNumberOfCalls();
		}
		if(node->isUnwound()) {
			report.unwindSamples += node->getExpectedNumberOfSamples();
		}
	}

	report.instrumentationOverhead = report.instrumentedCalls * nanosPerInstrumentedCall;
	report.unwindOverhead = report.unwindSamples * nanosPerUnwindSample;

	report.phaseName = name;
}

CgReport EstimatorPhase::getReport() {
	return this->report;
}

//// INSTRUMENT ESTIMATOR PHASE

InstrumentEstimatorPhase::InstrumentEstimatorPhase(
		std::map<std::string, std::shared_ptr<CgNode> >* graph) :
		EstimatorPhase(graph, "Instrument") {
}

InstrumentEstimatorPhase::~InstrumentEstimatorPhase() {
}

void InstrumentEstimatorPhase::modifyGraph(std::shared_ptr<CgNode> mainMethod) {
	if (mainMethod == NULL) {
		std::cerr << "Received NULL as main method." << std::endl;
		return;
	}

	std::queue<std::shared_ptr<CgNode> > workQueue;
	std::set<std::shared_ptr<CgNode> > doneNodes;

	workQueue.push(mainMethod);
	while (!workQueue.empty()) {

		auto node = workQueue.front();
		workQueue.pop();
		doneNodes.insert(node);

		if (CgHelper::isConjunction(node)) {
			for (auto nodeToInstrument : node->getParentNodes()) {
				nodeToInstrument->setState(CgNodeState::INSTRUMENT);
			}
		}

		// add child to work queue if not done yet
		for (auto childNode : node->getChildNodes()) {
			if(doneNodes.find(childNode) == doneNodes.end()) {
				workQueue.push(childNode);
			}
		}
	}
}

//// MOVE INSTRUMENTATION UPWARDS ESTIMATOR PHASE

MoveInstrumentationUpwardsEstimatorPhase::MoveInstrumentationUpwardsEstimatorPhase(
		std::map<std::string, std::shared_ptr<CgNode> >* graph) :
		EstimatorPhase(graph, "MoveInstrumentationUpwards") {
}

MoveInstrumentationUpwardsEstimatorPhase::~MoveInstrumentationUpwardsEstimatorPhase() {
}

void MoveInstrumentationUpwardsEstimatorPhase::modifyGraph(std::shared_ptr<CgNode> mainMethod) {
	for (auto graphPair : (*graph)) {
		// If the node was not selected previously, we continue
		if (!graphPair.second->isInstrumented()) {
			continue;
		}

		// If it was selected, we try to move the hook upwards
		auto cur = graphPair.second;
		while (cur->getParentNodes().size() == 1) {
			if ((*cur->getParentNodes().begin())->getChildNodes().size()>1) {
				break;
			}

			cur = *cur->getParentNodes().begin(); // This should be safe...
		}
		graphPair.second->setState(CgNodeState::NONE);
		cur->setState(CgNodeState::INSTRUMENT);
	}
}

//// UNWIND ESTIMATOR PHASE

UnwindEstimatorPhase::UnwindEstimatorPhase(
		std::map<std::string, std::shared_ptr<CgNode> >* graph) :
		EstimatorPhase(graph, "Unwind") {
}

UnwindEstimatorPhase::~UnwindEstimatorPhase() {
}

void UnwindEstimatorPhase::modifyGraph(std::shared_ptr<CgNode> mainMethod) {

	for (auto pair : (*graph)) {
		auto node = pair.second;

		// select all leaves
		if (node->isLeafNode() && CgHelper::isConjunction(node)) {	// TODO check if unwind is valid on this node
			node->setState(CgNodeState::UNWIND);
		}

		// remove redundant instrumentation
		for (auto parentNode : node->getParentNodes()) {

			bool redundantInstrumentation = true;
			for (auto childOfParentNode : parentNode->getChildNodes()) {

				if (!childOfParentNode->isUnwound()
						&& CgHelper::isConjunction(childOfParentNode)) {
					redundantInstrumentation = false;
				}
			}

			if (redundantInstrumentation) {
				parentNode->setState(CgNodeState::NONE);
			}
		}
	}
}

