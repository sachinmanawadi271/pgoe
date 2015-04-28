
#include "EstimatorPhase.h"

EstimatorPhase::EstimatorPhase(std::string name) :

		graph(NULL),	// just so eclipse does not nag
		report({0}),	// this hopefully initializes all members to 0
		name(name) {
}

void EstimatorPhase::generateReport() {

	for(auto node : (*graph)) {

		if(node->isInstrumented()) {
			report.instrumentedMethods += 1;
			report.instrumentedCalls += node->getNumberOfCalls();
		}
		if(node->isUnwound()) {
			unsigned long long unwindSamples = node->getExpectedNumberOfSamples();
			unsigned long long unwindSteps = node->getNumberOfUnwindSteps();

			double unwindCosts = unwindSamples *
					(CgConfig::nanosPerUnwindSample + unwindSteps * CgConfig::nanosPerUnwindStep);

			report.unwindSamples += unwindSamples;
			report.unwindOverhead += unwindCosts;
		}
	}

	report.overallMethods = graph->size();

	report.instrumentationOverhead = report.instrumentedCalls * CgConfig::nanosPerInstrumentedCall;

	report.phaseName = name;
}

void EstimatorPhase::setGraph(CgNodePtrSet* graph) {
	this->graph = graph;
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

RemoveUnrelatedNodesEstimatorPhase::RemoveUnrelatedNodesEstimatorPhase() :
		EstimatorPhase("RemoveUnrelated"),
		numRemovedNodes(0) {
}

RemoveUnrelatedNodesEstimatorPhase::~RemoveUnrelatedNodesEstimatorPhase() {
}

void RemoveUnrelatedNodesEstimatorPhase::modifyGraph(CgNodePtr mainMethod) {
	if (mainMethod == NULL) {
		std::cerr << "Received NULL as main method." << std::endl;
		return;
	}

	CgNodePtrSet visitedNodes;
	std::queue<CgNodePtr> workQueue;

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

	for (auto node : (*graph)) {
		// remove nodes that were not reachable from the main method
		if (visitedNodes.find(node) == visitedNodes.end()) {
			graph->erase(node);
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

//// GRAPH STATS ESTIMATOR PHASE

GraphStatsEstimatorPhase::GraphStatsEstimatorPhase() :
	EstimatorPhase("GraphStats"),
	numberOfConjunctions(0) {
}

GraphStatsEstimatorPhase::~GraphStatsEstimatorPhase() {
}

void GraphStatsEstimatorPhase::printReport() {
	// only print the additional report
	printAdditionalReport();
}

void GraphStatsEstimatorPhase::modifyGraph(CgNodePtr mainMethod) {

	for (auto node : (*graph)) {

		if (!CgHelper::isConjunction(node)) {
			continue;
		}

		numberOfConjunctions++;
		if (hasDependencyFor(node)) {
			continue;
		}

		CgNodePtrSet dependentConjunctions = {node};
		CgNodePtrSet validMarkerPositions = CgHelper::getPotentialMarkerPositions(node);

		unsigned int numberOfDependentConjunctions = 0;
		while (numberOfDependentConjunctions != dependentConjunctions.size()) {
			numberOfDependentConjunctions = dependentConjunctions.size();

			CgNodePtrSet reachableConjunctions = CgHelper::getReachableConjunctions(validMarkerPositions);
			for (auto depConj : dependentConjunctions) {
				reachableConjunctions.erase(depConj);
			}

			for (auto reachableConjunction : reachableConjunctions) {
				CgNodePtrSet otherValidMarkerPositions = CgHelper::getPotentialMarkerPositions(reachableConjunction);

				if (!CgHelper::setIntersect(validMarkerPositions, otherValidMarkerPositions).empty()) {
					dependentConjunctions.insert(reachableConjunction);
					validMarkerPositions.insert(otherValidMarkerPositions.begin(), otherValidMarkerPositions.end());
				}
			}
		}

		dependencies.push_back(ConjunctionDependency(dependentConjunctions, validMarkerPositions));
	}
}

void GraphStatsEstimatorPhase::printAdditionalReport() {
	std::cout << "==" << report.phaseName << "== Phase Report " << std::endl;
	std::cout << "\t" << "numberOfConjunctions: " << numberOfConjunctions << std::endl;
	for (auto dependency : dependencies) {
		std::cout << "\t- dependentConjunctions: " << std::setw(3) << dependency.dependentConjunctions.size()
				<< " | validMarkerPositions: " << std::setw(3) << dependency.markerPositions.size() << std::endl;
	}
}

//// INSTRUMENT ESTIMATOR PHASE

InstrumentEstimatorPhase::InstrumentEstimatorPhase() :
	EstimatorPhase("Instrument") {
}

InstrumentEstimatorPhase::~InstrumentEstimatorPhase() {
}

void InstrumentEstimatorPhase::modifyGraph(CgNodePtr mainMethod) {
	if (mainMethod == NULL) {
		std::cerr << "Received NULL as main method." << std::endl;
		return;
	}

	std::queue<CgNodePtr> workQueue;
	CgNodePtrSet doneNodes;

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

MoveInstrumentationUpwardsEstimatorPhase::MoveInstrumentationUpwardsEstimatorPhase() :
		EstimatorPhase("MoveInstrumentationUpwards"),
		movedInstrumentations(0) {
}

MoveInstrumentationUpwardsEstimatorPhase::~MoveInstrumentationUpwardsEstimatorPhase() {
}

void MoveInstrumentationUpwardsEstimatorPhase::modifyGraph(CgNodePtr mainMethod) {

	for (auto node : (*graph)) {

		auto nextAncestor = node;

		// If the node was not selected previously, we continue
		if (!nextAncestor->isInstrumented()) {
			continue;
		}

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

		if (!minimalCalls->isSameFunction(nextAncestor)) {
			node->setState(CgNodeState::NONE);
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

DeleteOneInstrumentationEstimatorPhase::DeleteOneInstrumentationEstimatorPhase() :
		EstimatorPhase("DeleteOneInstrumentation"),
		deletedInstrumentationMarkers(0) {
}

DeleteOneInstrumentationEstimatorPhase::~DeleteOneInstrumentationEstimatorPhase() {
}

void DeleteOneInstrumentationEstimatorPhase::modifyGraph(CgNodePtr mainMethod) {

	for (auto node : (*graph)) {

		if (!CgHelper::isConjunction(node) || node->isUnwound()) {
			continue;
		}

		if (!CgHelper::allParentsPathsInstrumented(node)) {
			continue;
		}

		unsigned long long expensivePath = 0;
		CgNodePtr mostExpensiveParent = 0;

		// XXX RN: the heuristic is far from perfect and might block another node with the same parent
		for (auto parentNode : node->getParentNodes()) {
			auto pathCosts = CgHelper::getInstrumentationOverheadOfPath(parentNode);
			// for some strange reason there are edges with 0 calls in the spec profiles
			if (pathCosts > expensivePath && CgHelper::instrumentationCanBeDeleted(parentNode)) {
				mostExpensiveParent = parentNode;
			}
		}

		if(mostExpensiveParent) {
			CgHelper::removeInstrumentationOnPath(mostExpensiveParent);
			deletedInstrumentationMarkers++;
		}
	}
}

void DeleteOneInstrumentationEstimatorPhase::printAdditionalReport() {
	std::cout << "\t" << "deleted " << deletedInstrumentationMarkers << " instrumentation marker(s)" << std::endl;
}

//// UNWIND ESTIMATOR PHASE

UnwindEstimatorPhase::UnwindEstimatorPhase() :
		EstimatorPhase("Unwind"),
		unwoundNodes(0),
		unwindCandidates(0) {
}

UnwindEstimatorPhase::~UnwindEstimatorPhase() {
}

void UnwindEstimatorPhase::modifyGraph(CgNodePtr mainMethod) {

	for (auto node : (*graph)) {

		// first select all leafs that are conjunctions
		if (node->isLeafNode() && CgHelper::isConjunction(node)) {

			unwindCandidates++;

			// TODO: use the actual benefit (with remaining instrumentation)
			// TODO: consider inserting multiple parallel unwind nodes at the same time, accumulate overhead
			unsigned long long expectedUnwindOverheadNanos =
					node->getExpectedNumberOfSamples() * (CgConfig::nanosPerUnwindSample + CgConfig::nanosPerUnwindStep);

			unsigned long long expectedInstrumentationOverheadNanos =
					CgHelper::getInstrumentationOverheadOfConjunction(node);

			if (expectedUnwindOverheadNanos < expectedInstrumentationOverheadNanos) {

				node->setState(CgNodeState::UNWIND, 1);
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
	std::cout << "\t" << "unwound " << unwoundNodes << " leaf node(s) of "
			<< unwindCandidates << " candidate(s)" << std::endl;
}

//// UNWIND ESTIMATOR PHASE

ResetEstimatorPhase::ResetEstimatorPhase() :
		EstimatorPhase("Reset") {
}

ResetEstimatorPhase::~ResetEstimatorPhase() {
}

void ResetEstimatorPhase::modifyGraph(CgNodePtr mainMethod) {
	for (auto node : (*graph)) {
		node->reset();
	}
}

void ResetEstimatorPhase::printAdditionalReport() {
	std::cout << std::endl;
}

