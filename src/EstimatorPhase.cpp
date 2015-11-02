
#include "EstimatorPhase.h"

EstimatorPhase::EstimatorPhase(std::string name) :

		graph(nullptr),	// just so eclipse does not nag
		report(),	// initializes all members of report
		name(name) {
}

void EstimatorPhase::generateReport() {

	for(auto node : (*graph)) {

		if(node->isInstrumented()) {
			report.instrumentedMethods += 1;
			report.instrumentedCalls += node->getNumberOfCalls();

			report.instrumentedNames.insert(node->getFunctionName());
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

	assert(report.instrumentedMethods == report.instrumentedNames.size());
}

void EstimatorPhase::setGraph(Callgraph* graph) {
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

RemoveUnrelatedNodesEstimatorPhase::RemoveUnrelatedNodesEstimatorPhase(bool onlyRemoveUnrelatedNodes, bool aggressiveReduction) :
		EstimatorPhase("RemoveUnrelated"),
		numUnconnectedRemoved(0),
		numLeafsRemoved(0),
		numChainsRemoved(0),
		numAdvancedOptimizations(0),
		aggressiveReduction(aggressiveReduction),
		onlyRemoveUnrelatedNodes(onlyRemoveUnrelatedNodes) {
}

RemoveUnrelatedNodesEstimatorPhase::~RemoveUnrelatedNodesEstimatorPhase() {
	nodesToRemove.clear();
}

void RemoveUnrelatedNodesEstimatorPhase::modifyGraph(CgNodePtr mainMethod) {
	if (mainMethod == nullptr) {
		std::cerr << "Received nullptr as main method." << std::endl;
		return;
	}

	CgNodePtrSet nodesReachableFromMain;
	std::queue<CgNodePtr> workQueue;

	/** XXX RN: code duplication */
	workQueue.push(mainMethod);
	while(!workQueue.empty()) {

		auto node = workQueue.front();
		workQueue.pop();
		nodesReachableFromMain.insert(node);

		for (auto childNode : node->getChildNodes()) {
			if (nodesReachableFromMain.find(childNode) == nodesReachableFromMain.end()) {
				workQueue.push(childNode);
			}
		}
	}

	for (auto node : (*graph)) {
		// remove nodes that were not reachable from the main method
		if (nodesReachableFromMain.find(node) == nodesReachableFromMain.end()) {
			graph->erase(node, false, true);
			numUnconnectedRemoved++;
			continue;
		}
	}

	if (onlyRemoveUnrelatedNodes) {
		return;
	}

	for (auto node : (*graph)) {
		// leaf nodes that are never unwound or instrumented
		if (node->isLeafNode()) {
			checkLeafNodeForRemoval(node);
		}
	}
	// actually remove those nodes
	for (auto node : nodesToRemove) {
		graph->erase(node);
	}

	for (auto node : (*graph)) {
		// reduce chain
		if (node->hasUniqueChild()) {

			if (!aggressiveReduction && CgHelper::isConjunction(node)) {
				continue;
			}

			auto uniqueChild = node->getUniqueChild();

			if (CgHelper::hasUniqueParent(uniqueChild)
					&& (node->getDependentConjunctions() == uniqueChild->getDependentConjunctions())
					&& !CgHelper::isOnCycle(node)) {

				numChainsRemoved++;

				if (node->getNumberOfCalls() >= uniqueChild->getNumberOfCalls()) {
					graph->erase(node, true);
				} else {
					graph->erase(uniqueChild, true);
				}
			}
		}
	}

	for (auto node : (*graph)) {
		// advanced optimization
		if (node->hasUniqueParent()	&& node->hasUniqueChild()
				&& node->getUniqueParent()->getNumberOfCalls() <= node->getNumberOfCalls() ) {

			CgNodePtrSet intersect = CgHelper::setIntersect(node->getUniqueParent()->getDependentConjunctions(), node->getDependentConjunctions());
			if (intersect == node->getDependentConjunctions()) {
				// TODO: also the two nodes have to serve the same conjunctions

				numAdvancedOptimizations++;
				graph->erase(node, true);
			}
		}
	}

}

void RemoveUnrelatedNodesEstimatorPhase::checkLeafNodeForRemoval(CgNodePtr node) {

	for (auto child : node->getChildNodes()) {
		if (nodesToRemove.find(child) == nodesToRemove.end()) {
			return;	// if a single child is not deleted yet, this node cannot be deleted anyways
		}
	}

	if (node->hasUniqueCallPath()
			|| (aggressiveReduction && node->hasUniqueParent()) ) {
		nodesToRemove.insert(node);
		numLeafsRemoved++;

		for (auto parentNode : node->getParentNodes()) {
			checkLeafNodeForRemoval(parentNode);
		}
	}
}

void RemoveUnrelatedNodesEstimatorPhase::printReport() {
	// only print the additional report
	printAdditionalReport();
}

void RemoveUnrelatedNodesEstimatorPhase::printAdditionalReport() {
	std::cout << "==" << report.phaseName << "== Phase Report " << std::endl;
	std::cout << "\t" << "Removed " << numUnconnectedRemoved << " unconnected node(s)."	<< std::endl;
	std::cout << "\t" << "Removed " << numLeafsRemoved << " leaf node(s)."	<< std::endl;
	std::cout << "\t" << "Removed " << numChainsRemoved << " node(s) in linear call chains."	<< std::endl;
	std::cout << "\t" << "Removed " << numAdvancedOptimizations << " node(s) in advanced optimization."	<< std::endl;
}

//// GRAPH STATS ESTIMATOR PHASE

GraphStatsEstimatorPhase::GraphStatsEstimatorPhase() :
	EstimatorPhase("GraphStats"),
	numCyclesDetected(0),
	numberOfConjunctions(0) {
}

GraphStatsEstimatorPhase::~GraphStatsEstimatorPhase() {
}

void GraphStatsEstimatorPhase::printReport() {
	// only print the additional report
	printAdditionalReport();
}

void GraphStatsEstimatorPhase::modifyGraph(CgNodePtr mainMethod) {

	// detect cycles
	for (auto node : (*graph)) {
		if (CgHelper::isOnCycle(node)) {
			numCyclesDetected++;
		}
	}

	// dependent conjunctions
	for (auto node : (*graph)) {

		if (!CgHelper::isConjunction(node)) {
			continue;
		}

		numberOfConjunctions++;
		if (hasDependencyFor(node)) {
			continue;
		}

		CgNodePtrSet dependentConjunctions = {node};
		CgNodePtrSet validMarkerPositions = node->getMarkerPositions();

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

		///XXX
//		if (dependentConjunctions.size() > 100) {
//			for (auto& dep : dependentConjunctions) {
//				graph->erase(dep, false, true);
//			}
//			for (auto& marker: allValidMarkerPositions) {
//				graph->erase(marker, false, true);
//			}
//		}

		dependencies.push_back(ConjunctionDependency(dependentConjunctions, validMarkerPositions));
		allValidMarkerPositions.insert(validMarkerPositions.begin(), validMarkerPositions.end());
	}
}

void GraphStatsEstimatorPhase::printAdditionalReport() {
	std::cout << "==" << report.phaseName << "== Phase Report " << std::endl;
	std::cout << "\t" << "nodes in cycles: " << numCyclesDetected << std::endl;
	std::cout << "\t" << "numberOfConjunctions: " << numberOfConjunctions
			<< " | allValidMarkerPositions: " << allValidMarkerPositions.size() << std::endl;
	for (auto dependency : dependencies) {
		std::cout << "\t- dependentConjunctions: " << std::setw(3) << dependency.dependentConjunctions.size()
				<< " | validMarkerPositions: " << std::setw(3) << dependency.markerPositions.size() << std::endl;
	}
}

//// DIAMOND PATTERN SOLVER ESTIMATOR PHASE

DiamondPatternSolverEstimatorPhase::DiamondPatternSolverEstimatorPhase() :
	EstimatorPhase("DiamondPattern"),
	numDiamonds(0),
	numUniqueConjunction(0),
	numOperableConjunctions(0) {
}

DiamondPatternSolverEstimatorPhase::~DiamondPatternSolverEstimatorPhase() {
}

void DiamondPatternSolverEstimatorPhase::modifyGraph(CgNodePtr mainMethod) {
	for (auto node : (*graph)) {
		if (CgHelper::isConjunction(node)) {
			for (auto parent1 : node->getParentNodes()) {

				if (parent1->hasUniqueParent() && parent1->hasUniqueChild()) {
					auto grandParent = parent1->getUniqueParent();

					for (auto parent2 : node->getParentNodes()) {
						if (parent1==parent2) {
							continue;
						}

						if (parent2->hasUniqueParent()
								&& parent2->getUniqueParent()==grandParent) {

							numDiamonds++;
							break;
						}

					}
				}
			}
		}
	}

	// conjunctions with a unique solution
	for (auto& node : (*graph)) {
		if (CgHelper::isConjunction(node)) {
			int numParents = node->getParentNodes().size();
			int numPossibleMarkerPositions = node->getMarkerPositions().size();

			assert(numPossibleMarkerPositions >= (numParents -1));

			if (numPossibleMarkerPositions == (numParents-1)) {

				///XXX instrument parents
				for (auto& marker : node->getMarkerPositions()) {
					marker->setState(CgNodeState::INSTRUMENT);
				}

				numUniqueConjunction++;
			}

			if (numPossibleMarkerPositions == numParents) {
				numOperableConjunctions++;
			}
		}
	}
}

void DiamondPatternSolverEstimatorPhase::printReport() {
	// only print the additional report
	printAdditionalReport();
}

void DiamondPatternSolverEstimatorPhase::printAdditionalReport() {
	std::cout << "==" << report.phaseName << "== Phase Report " << std::endl;
	std::cout << "\t" << "numberOfDiamonds: " << numDiamonds << std::endl;
	std::cout << "\t" << "numUniqueConjunction: " << numUniqueConjunction << std::endl;
	std::cout << "\t" << "numOperableConjunctions: " << numOperableConjunctions << std::endl;
}

//// INSTRUMENT ESTIMATOR PHASE

InstrumentEstimatorPhase::InstrumentEstimatorPhase() :
	EstimatorPhase("Instrument") {
}

InstrumentEstimatorPhase::~InstrumentEstimatorPhase() {
}

void InstrumentEstimatorPhase::modifyGraph(CgNodePtr mainMethod) {
	if (mainMethod == nullptr) {
		std::cerr << "Received nullptr as main method." << std::endl;
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
//		if (node->isLeafNode() && CgHelper::isConjunction(node)) {
		if (CgHelper::isConjunction(node)) {

			unwindCandidates++;

			// TODO: use the actual benefit (with remaining instrumentation)
			// TODO: consider inserting multiple parallel unwind nodes at the same time, accumulate overhead
			unsigned long long expectedUnwindSampleOverheadNanos =
					node->getExpectedNumberOfSamples() * (CgConfig::nanosPerUnwindSample + CgConfig::nanosPerUnwindStep);

			unsigned long long expectedUnwindInstrumentOverheadNanos =
					node->getNumberOfCalls() * (CgConfig::nanosPerUnwindSample + CgConfig::nanosPerUnwindStep);

			unsigned long long expectedInstrumentationOverheadNanos =
					CgHelper::getInstrumentationOverheadOfConjunction(node);

			unsigned long long expectedActualInstrumentationSavedNanos =
					CgHelper::getInstrumentationOverheadServingOnlyThisConjunction(node);

			unsigned long long unwindOverhead = expectedUnwindSampleOverheadNanos;
			unsigned long long instrumentationOverhead = expectedActualInstrumentationSavedNanos;

			if (unwindOverhead < instrumentationOverhead) {

				///XXX
				double expectedOverheadSavedSeconds
						= ((long long) instrumentationOverhead - (long long)unwindOverhead) / 1000000000.0;
				std::cout << std::setprecision(4) << expectedOverheadSavedSeconds << "s\t save expected in: " << node->getFunctionName() << std::endl;

//				node->setState(CgNodeState::UNWIND, 1);
				unwoundNodes++;

//			// remove redundant instrumentation in direct parents
//				for (auto parentNode : node->getParentNodes()) {
//
//					bool redundantInstrumentation = true;
//					for (auto childOfParentNode : parentNode->getChildNodes()) {
//
//						if (!childOfParentNode->isUnwound()
//								&& CgHelper::isConjunction(childOfParentNode)) {
//							redundantInstrumentation = false;
//						}
//					}
//
//					if (redundantInstrumentation) {
//						CgHelper::removeInstrumentationOnPath(parentNode);
//					}
//				}
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

