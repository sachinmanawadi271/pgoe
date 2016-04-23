
#include "EstimatorPhase.h"
#include <iomanip>


#define NO_DEBUG

EstimatorPhase::EstimatorPhase(std::string name, bool isMetaPhase) :

		graph(nullptr),	// just so eclipse does not nag
		report(),	// initializes all members of report
		name(name),
		config(nullptr),
		noReportRequired(isMetaPhase) {
}

void EstimatorPhase::generateReport() {

	for(auto node : (*graph)) {

		if(node->isInstrumented()) {
			report.instrumentedMethods += 1;
			report.instrumentedCalls += node->getNumberOfCalls();

			report.instrumentedNames.insert(node->getFunctionName());
			report.instrumentedNodes.push(node);
		}
		if(node->isUnwound()) {
			unsigned long long unwindSamples = 0;
			if (node->isUnwoundInstr()) {
				unwindSamples = node->getNumberOfCalls();
			} else if (node->isUnwoundSample()) {
				unwindSamples	= node->getExpectedNumberOfSamples();
			} else {
				std::cerr << "Error in generateRepor." << std::endl;
			}
			unsigned long long unwindSteps = node->getNumberOfUnwindSteps();

			unsigned long long unwindCostsNanos = unwindSamples *
					(CgConfig::nanosPerUnwindSample + unwindSteps * CgConfig::nanosPerUnwindStep);

			report.unwindSamples += unwindSamples;
			report.unwindOvSeconds += (double) unwindCostsNanos / 1e9;

			report.unwConjunctions++;
		}
		if (CgHelper::isConjunction(node)) {
			report.overallConjunctions++;
		}
	}

	report.overallMethods = graph->size();

	report.instrOvSeconds = (double) report.instrumentedCalls * CgConfig::nanosPerInstrumentedCall / 1e9;

	if (config->referenceRuntime > .0) {
 		report.instrOvPercent = report.instrOvSeconds / config->referenceRuntime * 100;
 		report.unwindOvPercent = report.unwindOvSeconds / config->referenceRuntime * 100;

 		report.samplesTaken = config->referenceRuntime * CgConfig::samplesPerSecond;
	} else {
		report.instrOvPercent = 0;
		report.samplesTaken = 0;
	}
	// sampling overhead
	report.samplingOvSeconds = report.samplesTaken * CgConfig::nanosPerSample / 1e9;
	report.samplingOvPercent = (double) (CgConfig::nanosPerSample * CgConfig::samplesPerSecond) / 1e7;

	report.overallSeconds = report.instrOvSeconds + report.unwindOvSeconds + report.samplingOvSeconds;
	report.overallPercent = report.instrOvPercent + report.unwindOvPercent + report.samplingOvPercent;

	report.metaPhase = noReportRequired;
	report.phaseName = name;

	if (!noReportRequired && report.overallPercent < config->fastestPhaseOvPercent) {
		config->fastestPhaseOvPercent = report.overallPercent;
		config->fastestPhaseOvSeconds = report.overallSeconds;
		config->fastestPhaseName = report.phaseName;
	}

	assert(report.instrumentedMethods == report.instrumentedNames.size());
	assert(report.instrumentedMethods == report.instrumentedNodes.size());
}

void EstimatorPhase::setGraph(Callgraph* graph) {
	this->graph = graph;
}

CgReport EstimatorPhase::getReport() {
	return this->report;
}

void EstimatorPhase::printReport() {

	if (config->tinyReport) {
		if (!report.metaPhase) {
			double overallOvPercent = report.instrOvPercent + report.unwindOvPercent + report.samplingOvPercent;
			std::cout << "==" << report.phaseName << "==  " << overallOvPercent
					<< " %" << std::endl;
		}
	} else {
		std::cout << "==" << report.phaseName << "==  " << std::endl;
		printAdditionalReport();
	}
}

void EstimatorPhase::printAdditionalReport() {

if (report.instrumentedCalls > 0) {
	std::cout
			<< " INSTR \t" <<std::setw(8) << std::left << report.instrOvPercent << " %"
			<< " | instr. " << report.instrumentedMethods << " of " << report.overallMethods << " methods"
			<< " | instrCalls: " << report.instrumentedCalls
			<< " | instrOverhead: " << report.instrOvSeconds << " s" << std::endl;
}
if (report.unwindSamples > 0) {
	std::cout
			<< "   UNW \t" << std::setw(8) << report.unwindOvPercent << " %"
			<< " | unwound " << report.unwConjunctions << " of " << report.overallConjunctions << " conj."
			<< " | unwindSamples: " << report.unwindSamples
			<< " | undwindOverhead: " << report.unwindOvSeconds << " s" << std::endl;
}
	std::cout
			<< " SAMPL \t" << std::setw(8)  << report.samplingOvPercent << " %"
			<< " | taken samples: " << report.samplesTaken
			<< " | samplingOverhead: " << report.samplingOvSeconds << " s" << std::endl

			<< " ---->\t" <<std::setw(8) << report.overallPercent << " %"
			<< " | overallOverhead: " << report.overallSeconds << " s"
			<< std::endl;
}

//// REMOVE UNRELATED NODES ESTIMATOR PHASE

RemoveUnrelatedNodesEstimatorPhase::RemoveUnrelatedNodesEstimatorPhase(bool onlyRemoveUnrelatedNodes, bool aggressiveReduction) :
		EstimatorPhase("RemoveUnrelated", true),
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

	/* remove unrelated nodes */
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

	/* remove leaf nodes */
	for (auto node : (*graph)) {
		if (node->isLeafNode()) {
			checkLeafNodeForRemoval(node);
		}
	}
	// actually remove those nodes
	for (auto node : nodesToRemove) {
		graph->erase(node);
	}

///XXX
	for (auto node : (*graph)) {
		if (node->isLeafNode() && !CgHelper::isConjunction(node)) {
			std::cout << "####################WTF " << node->getFunctionName() << std::endl;
		}
	}


	/* remove linear chains */
	for (auto node : (*graph)) {
		if (node->hasUniqueChild()) {

			if (CgHelper::isConjunction(node)) {
				continue;
			}

			auto uniqueChild = node->getUniqueChild();

			if (CgHelper::hasUniqueParent(uniqueChild)
					&& (node->getDependentConjunctionsConst() == uniqueChild->getDependentConjunctionsConst())
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

	if (!aggressiveReduction) {
		return;
	}

	CgNodePtrQueueMostCalls allNodes(graph->begin(), graph->end());
	for (auto node : Container(allNodes)) {

		// advanced optimization (remove node with subset of dependentConjunctions
		if (node->hasUniqueParent() && node->hasUniqueChild()) {

			auto uniqueParent = node->getUniqueParent();
			bool uniqueParentHasLessCalls = uniqueParent->getNumberOfCalls() <= node->getNumberOfCalls();
			bool uniqueParentServesMoreOrEqualsNodes = CgHelper::isSubsetOf(node->getDependentConjunctionsConst(),
					uniqueParent->getDependentConjunctionsConst());

			bool aggressiveReductionPossible = true;
			if (!uniqueParent->hasUniqueChild()) {
				for (auto childOfParent : uniqueParent->getChildNodes()) {
					if (childOfParent != node && CgHelper::intersects(node->getDependentConjunctionsConst(),
									childOfParent->getDependentConjunctionsConst())) {
						aggressiveReductionPossible = false;
						break;
					}
				}
			}

			if (uniqueParentHasLessCalls && uniqueParentServesMoreOrEqualsNodes && aggressiveReductionPossible) {
				numAdvancedOptimizations++;
				graph->erase(node, true);
			}
		}
	}

}

void RemoveUnrelatedNodesEstimatorPhase::checkLeafNodeForRemoval(CgNodePtr potentialLeaf) {

	if (CgHelper::isConjunction(potentialLeaf)) {
		return;	// conjunctions are never removed
	}

	for (auto child : potentialLeaf->getChildNodes()) {
		if (nodesToRemove.find(child) == nodesToRemove.end()) {
			return;
		}
	}

	nodesToRemove.insert(potentialLeaf);
	numLeafsRemoved++;

	for (auto parentNode : potentialLeaf->getParentNodes()) {
		checkLeafNodeForRemoval(parentNode);
	}
}

void RemoveUnrelatedNodesEstimatorPhase::printAdditionalReport() {
	std::cout << "\t" << "Removed " << numUnconnectedRemoved << " unconnected node(s)."	<< std::endl;
	std::cout << "\t" << "Removed " << numLeafsRemoved << " leaf node(s)."	<< std::endl;
	std::cout << "\t" << "Removed " << numChainsRemoved << " node(s) in linear call chains."	<< std::endl;
	std::cout << "\t" << "Removed " << numAdvancedOptimizations << " node(s) in advanced optimization."	<< std::endl;
}

//// GRAPH STATS ESTIMATOR PHASE

GraphStatsEstimatorPhase::GraphStatsEstimatorPhase() :
	EstimatorPhase("GraphStats", true),
	numCyclesDetected(0),
	numberOfConjunctions(0) {
}

GraphStatsEstimatorPhase::~GraphStatsEstimatorPhase() {
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
	std::cout << "\t" << "nodes in cycles: " << numCyclesDetected << std::endl;
	std::cout << "\t" << "numberOfConjunctions: " << numberOfConjunctions
			<< " | allValidMarkerPositions: " << allValidMarkerPositions.size() << std::endl;
	for (auto dependency : dependencies) {
		std::cout << "\t- dependentConjunctions: " << std::setw(3) << dependency.dependentConjunctions.size()
				<< " | validMarkerPositions: " << std::setw(3) << dependency.markerPositions.size() << std::endl;
	}
}

//// OVERHEAD COMPENSATION ESTIMATOR PHASE

OverheadCompensationEstimatorPhase::OverheadCompensationEstimatorPhase(int nanosPerHalpProbe) :
	EstimatorPhase("OvCompensation", true),
	nanosPerHalpProbe(nanosPerHalpProbe),
	overallRuntime(0),
	numOvercompensatedFunctions(0) {}

void OverheadCompensationEstimatorPhase::modifyGraph(CgNodePtr mainMethod) {
	for (auto node : *graph) {
		auto oldRuntime = node->getRuntimeInSeconds();
		unsigned long long numberOfOwnOverheads = node->getNumberOfCalls();
		unsigned long long numberOfChildOverheads = 0;
		for (auto child : node->getChildNodes()) {
			numberOfChildOverheads += child->getNumberOfCalls(node);
		}

		unsigned long long timestampOverheadNanos = numberOfOwnOverheads * nanosPerHalpProbe + numberOfChildOverheads * nanosPerHalpProbe;
		double timestampOverheadSeconds = (double) timestampOverheadNanos / 1e9;
		double newRuntime = oldRuntime - timestampOverheadSeconds;

		if (newRuntime < 0) {
			node->setRuntimeInSeconds(0);
			numOvercompensatedFunctions++;
		} else {
			node->setRuntimeInSeconds(newRuntime);
		}

		node->updateExpectedNumberOfSamples();

		overallRuntime += newRuntime;
	}

	config->actualRuntime = overallRuntime;
}

void OverheadCompensationEstimatorPhase::printAdditionalReport() {
	std::cout << "\t" << "new runtime in seconds: " << overallRuntime
			<< " | overcompensated: " << numOvercompensatedFunctions
			<< std::endl;
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
					marker->setState(CgNodeState::INSTRUMENT_WITNESS);
				}

				numUniqueConjunction++;
			}

			if (numPossibleMarkerPositions == numParents) {
				numOperableConjunctions++;
			}
		}
	}
}

void DiamondPatternSolverEstimatorPhase::printAdditionalReport() {
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
				nodeToInstrument->setState(CgNodeState::INSTRUMENT_WITNESS);
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

//// WL INSTR ESTIMATOR PHASE

WLInstrEstimatorPhase::WLInstrEstimatorPhase(std::string wlFilePath) :
		EstimatorPhase("WLInstr") {

	std::ifstream ifStream(wlFilePath);
	if (!ifStream.good()) {
		std::cerr << "Error: can not find whitelist at .. " << wlFilePath << std::endl;
	}

	std::string buff;
	while (getline(ifStream, buff)) {
		whiteList.insert(buff);
	}
}

void WLInstrEstimatorPhase::modifyGraph(CgNodePtr mainMethod) {
	for (auto node : (*graph)) {
		if (whiteList.find(node->getFunctionName()) != whiteList.end()) {
			node->setState(CgNodeState::INSTRUMENT_WITNESS);
		}
	}
}

//// LIBUNWIND ESTIMATOR PHASE

LibUnwindEstimatorPhase::LibUnwindEstimatorPhase(bool unwindUntilUniqueCallpath) :
		EstimatorPhase(unwindUntilUniqueCallpath ? "LibUnwUnique" : "LibUnwStandard"),
		currentDepth(0),
		unwindUntilUniqueCallpath(unwindUntilUniqueCallpath) {}

void LibUnwindEstimatorPhase::modifyGraph(CgNodePtr mainMethod) {
	// find max distance from main for every function
	for (auto child : mainMethod->getChildNodes()) {
		visit (mainMethod, child);
	}
}

void LibUnwindEstimatorPhase::visit(CgNodePtr from, CgNodePtr current) {

	if (!(unwindUntilUniqueCallpath && current->hasUniqueCallPath())) {
		currentDepth++;
	}

	visitedEdges.insert(CgEdge(from, current));

	auto unwindSteps = current->getNumberOfUnwindSteps();
	if (currentDepth > unwindSteps) {
		current->setState(CgNodeState::UNWIND_SAMPLE, currentDepth);
	}

	for (CgNodePtr child : current->getChildNodes()) {
		if (visitedEdges.find(CgEdge(current, child)) == visitedEdges.end()) {
			visit(current, child);
		}
	}

	if (!(unwindUntilUniqueCallpath && current->hasUniqueCallPath())) {
		currentDepth--;
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
		if (!nextAncestor->isInstrumentedWitness()) {
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
			minimalCalls->setState(CgNodeState::INSTRUMENT_WITNESS);
			movedInstrumentations++;
		}
	}
}

//// DELETE ONE INSTRUMENTATION ESTIMATOR PHASE

DeleteOneInstrumentationEstimatorPhase::DeleteOneInstrumentationEstimatorPhase() :
		EstimatorPhase("DeleteOneInstrumentation"),
		deletedInstrumentationMarkers(0) {
}

DeleteOneInstrumentationEstimatorPhase::~DeleteOneInstrumentationEstimatorPhase() {
}

void DeleteOneInstrumentationEstimatorPhase::modifyGraph(CgNodePtr mainMethod) {

	CgNodePtrQueueMostCalls pq;
	for (auto node : (*graph)) {
		if (node->isInstrumentedWitness()) {
			pq.push(node);
		}
	}

	for (auto node : Container(pq)) {
		if (CgHelper::instrumentationCanBeDeleted(node)) {
			node->setState(CgNodeState::NONE);
			deletedInstrumentationMarkers++;
		}
	}
}

void DeleteOneInstrumentationEstimatorPhase::printAdditionalReport() {
	EstimatorPhase::printAdditionalReport();
	if (!config->tinyReport) {
		std::cout << "\t" << "deleted " << deletedInstrumentationMarkers
				<< " instrumentation marker(s)" << std::endl;
	}
}

//// CONJUNCTION ESTIMATOR PHASE

ConjunctionEstimatorPhase::ConjunctionEstimatorPhase(bool instrumentOnlyConjunctions) :
		EstimatorPhase(instrumentOnlyConjunctions ? "ConjunctionOnly" : "ConjunctionOrWitness"), instrumentOnlyConjunctions(instrumentOnlyConjunctions) {}

ConjunctionEstimatorPhase::~ConjunctionEstimatorPhase() {}

void ConjunctionEstimatorPhase::modifyGraph(CgNodePtr mainMethod) {

	if (instrumentOnlyConjunctions) {
		for (auto node : (*graph)) {
			if (CgHelper::isConjunction(node)) {
				node->setState(CgNodeState::INSTRUMENT_CONJUNCTION);
			}
		}
	} else {

		// either instrument all parents or the conjunction itself

		// TODO build this phase like unwinding estimation -> substitute instr with conj instr

		std::queue<CgNodePtr> workQueue;
		CgNodePtrSet doneNodes;

		/** XXX RN: code duplication */
		workQueue.push(mainMethod);
		while (!workQueue.empty()) {

			auto node = workQueue.front();
			workQueue.pop();
			doneNodes.insert(node);

			if (CgHelper::isConjunction(node)) {

				if (!CgHelper::isUniquelyInstrumented(node)) {
					unsigned long long conjunctionCallsToInstrument = node->getNumberOfCalls();
					unsigned long long parentCallsToInstrument = 0;
					for (auto parent : node->getParentNodes()) {
						if (!parent->isInstrumentedWitness()) {
							parentCallsToInstrument += parent->getNumberOfCalls();
						}
					}

					if (parentCallsToInstrument <= conjunctionCallsToInstrument) {
						for (auto parent : node->getParentNodes()) {
							parent->setState(CgNodeState::INSTRUMENT_WITNESS);
						}
					} else {
						node->setState(CgNodeState::INSTRUMENT_CONJUNCTION);
					}
				}
			}

			// add child to work queue if not done yet
			for (auto childNode : node->getChildNodes()) {
				if (doneNodes.find(childNode) == doneNodes.end()) {
					workQueue.push(childNode);
				}
			}
		}
	}
}

//// UNWIND ESTIMATOR PHASE

UnwindEstimatorPhase::UnwindEstimatorPhase(bool unwindOnlyLeafNodes, bool unwindInInstr) :
		EstimatorPhase(
				unwindOnlyLeafNodes ?
						(unwindInInstr ? "UnwindInstrLeaf" : "UnwindSampleLeaf") :
						(unwindInInstr ? "UnwindInstr" : "UnwindSample")),
		numUnwoundNodes(0),
		unwindCandidates(0),
		unwindOnlyLeafNodes(unwindOnlyLeafNodes),
		unwindInInstr(unwindInInstr) {
}

UnwindEstimatorPhase::~UnwindEstimatorPhase() {
}

/** XXX never call this if there are cycles in the successors of the start node */
void UnwindEstimatorPhase::getNewlyUnwoundNodes(std::map<CgNodePtr, int>& unwoundNodes, CgNodePtr startNode, int unwindSteps) {

	if (unwoundNodes.find(startNode) == unwoundNodes.end() || unwoundNodes[startNode] < unwindSteps) {
		unwoundNodes[startNode] = unwindSteps;
	}
	for (auto child : startNode->getChildNodes()) {
		getNewlyUnwoundNodes(unwoundNodes, child, unwindSteps+1);
	}
}

bool UnwindEstimatorPhase::canBeUnwound(CgNodePtr startNode) {

	if (unwindOnlyLeafNodes && !startNode->isLeafNode()) {
		return false;
	}

	for (auto node : CgHelper::getDescendants(startNode)) {
		if (CgHelper::isOnCycle(node)) {
			return false;
		}
	}
	return true;
}

unsigned long long UnwindEstimatorPhase::getUnwindOverheadNanos(std::map<CgNodePtr, int>& unwoundNodes) {
	unsigned long long unwindSampleOverheadNanos = 0;
	for (auto pair : unwoundNodes) {

		int numUnwindSteps = pair.second;
		int numExistingUnwindSteps = pair.first->getNumberOfUnwindSteps();

		int numNewUnwindSteps = 0;
		if(numUnwindSteps > numExistingUnwindSteps) {
			numNewUnwindSteps = numUnwindSteps - numExistingUnwindSteps;
		}

		unsigned long long numSamples = pair.first->getExpectedNumberOfSamples();
		unwindSampleOverheadNanos += numSamples * numNewUnwindSteps * CgConfig::nanosPerUnwindStep;
	}

	return unwindSampleOverheadNanos;
}

unsigned long long UnwindEstimatorPhase::getInstrOverheadNanos(std::map<CgNodePtr, int>& unwoundNodes) {
	// optimistic
	std::set<CgNodePtr> keySet;
	for (auto pair : unwoundNodes) { keySet.insert(pair.first); }

	unsigned long long expectedInstrumentationOverheadNanos =
			CgHelper::getInstrumentationOverheadOfConjunction(keySet);

	unsigned long long expectedActualInstrumentationSavedNanos =
			CgHelper::getInstrumentationOverheadServingOnlyThisConjunction(keySet);

	return (expectedInstrumentationOverheadNanos + expectedActualInstrumentationSavedNanos) / 2;
}

void UnwindEstimatorPhase::modifyGraph(CgNodePtr mainMethod) {
#ifndef NO_DEBUG
	double overallSavedSeconds = .0;
#endif
	for (auto node : (*graph)) {

		if (CgHelper::isConjunction(node) && canBeUnwound(node)) {
			unwindCandidates++;

			std::map<CgNodePtr, int> unwoundNodes;
			getNewlyUnwoundNodes(unwoundNodes, node);

			unsigned long long unwindOverhead = getUnwindOverheadNanos(unwoundNodes);;
			if (unwindInInstr) {
				unwindOverhead = node->getNumberOfCalls() * CgConfig::nanosPerUnwindStep;
			}

			unsigned long long instrumentationOverhead = getInstrOverheadNanos(unwoundNodes);

			if (unwindOverhead < instrumentationOverhead) {

#ifndef NO_DEBUG
				///XXX
				double expectedOverheadSavedSeconds
						= ((long long) instrumentationOverhead - (long long)unwindOverhead) / 1000000000.0;
				if (expectedOverheadSavedSeconds > 0.1) {
					if (!node->isLeafNode()) {
						std::cout << "No Leaf - " << unwoundNodes.size() << " nodes unwound" << std::endl;
					}
					std::cout << std::setprecision(4) << expectedOverheadSavedSeconds << "s\t save expected in: " << node->getFunctionName() << std::endl;
				}
				overallSavedSeconds += expectedOverheadSavedSeconds;
#endif

				if (unwindInInstr) {
					node->setState(CgNodeState::UNWIND_INSTR, 1);
					numUnwoundNodes++;
				} else {
					for (auto pair : unwoundNodes) {
						int numExistingUnwindSteps = pair.first->getNumberOfUnwindSteps();
						if (numExistingUnwindSteps == 0) {
							numUnwoundNodes++;
						}
						if (pair.second > numExistingUnwindSteps) {
							pair.first->setState(CgNodeState::UNWIND_SAMPLE, pair.second);
						}
					}
				}

			// remove redundant instrumentation in direct parents
				for (auto pair : unwoundNodes) {
					for (auto parentNode : pair.first->getParentNodes()) {

						bool redundantInstrumentation = true;
						for (auto childOfParentNode : parentNode->getChildNodes()) {

							if (!childOfParentNode->isUnwound()
									&& CgHelper::isConjunction(childOfParentNode)) {
								redundantInstrumentation = false;
							}
						}

						if (redundantInstrumentation) {
							if (parentNode->isInstrumented()) {
								parentNode->setState(CgNodeState::NONE);
							}
						}
					}
				}
			}
		}
	}

#ifndef NO_DEBUG
	std::cout << overallSavedSeconds << " s maximum save through unwinding." << std::endl;
#endif
}

//// UNWIND ESTIMATOR PHASE

ResetEstimatorPhase::ResetEstimatorPhase() :
		EstimatorPhase("Reset", true) {
}

ResetEstimatorPhase::~ResetEstimatorPhase() {
}

void ResetEstimatorPhase::modifyGraph(CgNodePtr mainMethod) {
	for (auto node : (*graph)) {
		node->reset();
	}
}

void ResetEstimatorPhase::printReport() {
	std::cout << "==" << report.phaseName << "== Phase " << std::endl << std::endl;
}


