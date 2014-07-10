
#include "EstimatorPhase.h"

EstimatorPhase::EstimatorPhase(
		std::map<std::string, std::shared_ptr<CgNode> >* graph, std::string name) :

		graph(graph),
		report({0}),// this hopefully initializes all members to 0
		name(name) {
}

void EstimatorPhase::generateReport() {

	for(auto pair : (*graph) ) {
		auto node = pair.second;

		if(node->isInstrumented()) {
			report.instrumentedMethods += 1;
			report.instrumentedCalls += node->getNumberOfCalls();
		}
		if(node->isUnwound()) {
			report.unwindSamples += node->getExpectedNumberOfSamples();
		}
	}

	report.overallMethods = graph->size();

	report.instrumentationOverhead = report.instrumentedCalls * CgConfig::nanosPerInstrumentedCall;
	report.unwindOverhead = report.unwindSamples * CgConfig::nanosPerUnwindSample;

	report.phaseName = name;
}

CgReport EstimatorPhase::getReport() {
	return this->report;
}

void EstimatorPhase::printReport() {

	double overallOverhead = report.instrumentationOverhead+report.unwindOverhead;

	std::cout << "==" << report.phaseName << "== Phase Report " << std::endl;
	std::cout << "\t" << "instrumented " << report.instrumentedMethods
			<< " of " << report.overallMethods << " methods" << std::endl
			<< "\t" << "instrumentedCalls: " << report.instrumentedCalls
			<< " | instrumentationOverhead: " << report.instrumentationOverhead << " ns" << std::endl
			<< "\t" << "unwindSamples: " << report.unwindSamples
			<< " | undwindOverhead: " << report.unwindOverhead << " ns" << std::endl
			<< "\t" << "overallOverhead: " << overallOverhead	<< " ns"
			<< " | that is: " << overallOverhead/1e9 <<" s"<< std::endl;

	printAdditionalReport();
}

//// REMOVE UNRELATED NODES ESTIMATOR PHASE

RemoveUnrelatedNodesEstimatorPhase::RemoveUnrelatedNodesEstimatorPhase(
		std::map<std::string, std::shared_ptr<CgNode> >* graph) :
		EstimatorPhase(graph, "RemoveUnrelated"),
		numRemovedNodes(0) {
}

RemoveUnrelatedNodesEstimatorPhase::~RemoveUnrelatedNodesEstimatorPhase() {
}

void RemoveUnrelatedNodesEstimatorPhase::modifyGraph(std::shared_ptr<CgNode> mainMethod) {
	if (mainMethod == NULL) {
		std::cerr << "Received NULL as main method." << std::endl;
		return;
	}

	std::set<std::shared_ptr<CgNode> > visitedNodes;
	std::queue<std::shared_ptr<CgNode> > workQueue;

	/** XXX RN: code duplication */
	workQueue.push(mainMethod);
	while(!workQueue.empty()) {

		auto node = workQueue.front();
		workQueue.pop();
		visitedNodes.insert(node);

		for (auto childNode : node->getChildNodes()) {
			if (visitedNodes.find(childNode) == visitedNodes.end()) {
				workQueue.push(childNode);
			}
		}
	}

	for (auto pair : (*graph)) {
		auto node = pair.second;

		// remove nodes that were not reachable from the main method
		if (visitedNodes.find(node) == visitedNodes.end()) {
			graph->erase(node->getFunctionName());
			numRemovedNodes++;
		}
	}


}

void RemoveUnrelatedNodesEstimatorPhase::printReport() {
	// only print the additional report
	printAdditionalReport();
}

void RemoveUnrelatedNodesEstimatorPhase::printAdditionalReport() {
	std::cout << "==" << report.phaseName << "== Phase Report " << std::endl;
	std::cout << "\t" << "Removed " << numRemovedNodes << " unrelated node(s)."	<< std::endl;
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

	/** XXX RN: code duplication */
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
		EstimatorPhase(graph, "MoveInstrumentationUpwards"),
		movedInstrumentations(0) {
}

MoveInstrumentationUpwardsEstimatorPhase::~MoveInstrumentationUpwardsEstimatorPhase() {
}

void MoveInstrumentationUpwardsEstimatorPhase::modifyGraph(
		std::shared_ptr<CgNode> mainMethod) {

	for (auto graphPair : (*graph)) {
		// If the node was not selected previously, we continue
		if (!graphPair.second->isInstrumented()) {
			continue;
		}

		auto nextAncestor = graphPair.second;
		auto minimalCalls = nextAncestor;

		// If it was selected, look for a "cheaper" node upwards
		while (CgHelper::hasUniqueParent(nextAncestor)) {

			nextAncestor = CgHelper::getUniqueParent(nextAncestor);
			if (nextAncestor->getChildNodes().size()>1) {
				break;	// don't move if parent has multiple children
			}

			if (nextAncestor->getNumberOfCalls() < minimalCalls->getNumberOfCalls()) {
				minimalCalls = nextAncestor;
			}
		}

		if (!minimalCalls->isSameFunction(graphPair.second)) {
			graphPair.second->setState(CgNodeState::NONE);
			minimalCalls->setState(CgNodeState::INSTRUMENT);
			movedInstrumentations++;
		}
	}
}

void MoveInstrumentationUpwardsEstimatorPhase::printAdditionalReport() {
	std::cout << "\t" << "moved " << movedInstrumentations
			<< " instrumentation marker(s)" << std::endl;
}

//// DELETE ONE INSTRUMENTATION ESTIMATOR PHASE

DeleteOneInstrumentationEstimatorPhase::DeleteOneInstrumentationEstimatorPhase(
		std::map<std::string, std::shared_ptr<CgNode> >* graph) :
		EstimatorPhase(graph, "DeleteOneInstrumentation"),
		deletedNodes(0) {
}

DeleteOneInstrumentationEstimatorPhase::~DeleteOneInstrumentationEstimatorPhase() {
}

void DeleteOneInstrumentationEstimatorPhase::modifyGraph(std::shared_ptr<CgNode> mainMethod) {
	// TODO RN: insert intelligent code here
}

void DeleteOneInstrumentationEstimatorPhase::printAdditionalReport() {
	std::cout << "\t" << "deleted " << deletedNodes << " instrumentation marker(s)" << std::endl;
}

//// UNWIND ESTIMATOR PHASE

UnwindEstimatorPhase::UnwindEstimatorPhase(
		std::map<std::string, std::shared_ptr<CgNode> >* graph) :
		EstimatorPhase(graph, "Unwind"),
		unwoundNodes(0),
		unwindCandidates(0) {
}

UnwindEstimatorPhase::~UnwindEstimatorPhase() {
}

void UnwindEstimatorPhase::modifyGraph(std::shared_ptr<CgNode> mainMethod) {

	for (auto pair : (*graph)) {
		auto node = pair.second;

		// first select all leafs that are conjunctions
		if (node->isLeafNode() && CgHelper::isConjunction(node)) {

			unwindCandidates++;

			// TODO: use the actual benefit (with remaining instrumentation)
			// TODO: consider inserting multiple parallel unwind nodes at the same time, accumulate overhead
			unsigned long long expectedUnwindOverheadNanos =
					node->getExpectedNumberOfSamples() * CgConfig::nanosPerUnwindSample;

			unsigned long long expectedInstrumentationOverheadNanos =
					CgHelper::getInstrumentationOverheadOfConjunction(node);

			if (expectedUnwindOverheadNanos < expectedInstrumentationOverheadNanos) {

				node->setState(CgNodeState::UNWIND);
				unwoundNodes++;

				// remove redundant instrumentation in direct parents
				for (auto parentNode : node->getParentNodes()) {

					bool redundantInstrumentation = true;
					for (auto childOfParentNode : parentNode->getChildNodes()) {

						if (!childOfParentNode->isUnwound()
								&& CgHelper::isConjunction(childOfParentNode)) {
							redundantInstrumentation = false;
						}
					}

					if (redundantInstrumentation) {
						CgHelper::removeInstrumentationOnPath(parentNode);
					}
				}
			}
		}
	}
}

void UnwindEstimatorPhase::printAdditionalReport() {
	std::cout << "\t" << "Unwound " << unwoundNodes << " leaf node(s) of "
			<< unwindCandidates << " candidate(s)" << std::endl;
}

