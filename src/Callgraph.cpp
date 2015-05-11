#include "Callgraph.h"

#include <chrono>	// easy time measurement

#define VERBOSE 0
#define DEBUG 0

#define BENCHMARK_PHASES 0
#define PRINT_FINAL_DOT 1
#define PRINT_DOT_AFTER_EVERY_PHASE 1

void Callgraph::insert(CgNodePtr node) {
	graph.insert(node);
}

void Callgraph::erase(CgNodePtr node, bool rewireAfterDeletion, bool force) {
	if (!force) {
		if (CgHelper::isConjunction(node) && node->getChildNodes().size() > 1) {
			std::cerr << "Error: Cannot remove node with multiple parents AND multiple children." << std::endl;
			exit(EXIT_FAILURE);
		}
		if (CgHelper::isConjunction(node) && node->isLeafNode()) {
			std::cerr << "Error: Cannot remove conjunction node that is a leaf." << std::endl;
			exit(EXIT_FAILURE);
		}
	}

	for (auto parent : node->getParentNodes()) {
		parent->removeChildNode(node);
	}
	for (auto child : node->getChildNodes()) {
		child->removeParentNode(node);
	}

	// a conjunction can only be erased if it has exactly one child
	if (!force && CgHelper::isConjunction(node)) {
		auto child = node->getUniqueChild();
		child->getMarkerPositions() = node->getMarkerPositions();

		for (auto markerPosition : node->getMarkerPositions()) {
			markerPosition->getDependentConjunctions().erase(node);
			markerPosition->getDependentConjunctions().insert(child);
		}
		// XXX erasing a node with multiple parents invalidates edge weights (they are visible in the dot output)
	}
	for (auto dependentConjunction : node->getDependentConjunctions()) {
		dependentConjunction->getMarkerPositions().erase(node);
	}

	if (rewireAfterDeletion) {
		for (auto parent : node->getParentNodes()) {
			for (auto child : node->getChildNodes()) {
				parent->addChildNode(child);
				child->addParentNode(parent);
			}
		}
	}

	graph.erase(node);

//	std::cout << "  Erasing node: " << *node << std::endl;
//	std::cout << "  UseCount: " << node.use_count() << std::endl;
}

CgNodePtrSet::iterator Callgraph::begin() {
	return graph.begin();
}
CgNodePtrSet::iterator Callgraph::end() {
	return graph.end();
}

size_t Callgraph::size() {
	return graph.size();
}

//// CALLGRAPH MANAGER

CallgraphManager::CallgraphManager(int samplesPerSecond) :
		samplesPerSecond(samplesPerSecond) {
}

CgNodePtr CallgraphManager::findOrCreateNode(std::string name, double timeInSeconds) {
	if (graphMapping.find(name) != graphMapping.end()) {
		return graphMapping.find(name)->second;
	} else {
		CgNodePtr node = std::make_shared<CgNode>(name);
		graphMapping.insert(std::pair<std::string, CgNodePtr>(name, node));
		graph.insert(node);

		node->setRuntimeInSeconds(timeInSeconds);

		return node;
	}
}

void CallgraphManager::putEdge(std::string parentName, std::string childName) {

	CgNodePtr parentNode = findOrCreateNode(parentName);
	CgNodePtr childNode = findOrCreateNode(childName);

	parentNode->addChildNode(childNode);
	childNode->addParentNode(parentNode);
}

void CallgraphManager::putEdge(std::string parentName, std::string parentFilename,
		int parentLine, std::string childName, unsigned long long numberOfCalls,
		double timeInSeconds) {

	putEdge(parentName, childName);

	auto parentNode = findNode(parentName);
	if (parentNode == NULL) {
		std::cerr << "ERROR in looking up node." << std::endl;
	}
	parentNode->setFilename(parentFilename);
	parentNode->setLineNumber(parentLine);

	auto childNode = findNode(childName);
	childNode->addCallData(parentNode, numberOfCalls, timeInSeconds);
}


CgNodePtr CallgraphManager::findMain() {
	return findNode("main");
}

CgNodePtr CallgraphManager::findNode(std::string functionName) {

	for (auto node : graph) {
		auto fName = node->getFunctionName();
		if (fName == functionName) {
			return node;
		}
	}
	return NULL;
}

void CallgraphManager::registerEstimatorPhase(EstimatorPhase* phase) {
	phases.push(phase);
	phase->setGraph(&graph);
}

void CallgraphManager::finalizeGraph() {
	graphMapping.clear();

	// also update all node attributes
	for (auto node : graph) {

		node->updateNodeAttributes(this->samplesPerSecond);

		CgNodePtrSet markerPositions = CgHelper::getPotentialMarkerPositions(node);
		node->getMarkerPositions().insert(markerPositions.begin(), markerPositions.end());

		std::for_each(markerPositions.begin(), markerPositions.end(),
				[&node](const CgNodePtr& markerPosition) { markerPosition->getDependentConjunctions().insert(node); });
	}
}

void CallgraphManager::thatOneLargeMethod() {

	finalizeGraph();

	while(!phases.empty()) {
		EstimatorPhase* phase = phases.front();

#if BENCHMARK_PHASES
		auto startTime = std::chrono::system_clock::now();
#endif
		phase->modifyGraph(findMain());
		phase->generateReport();

		phase->printReport();
#if PRINT_DOT_AFTER_EVERY_PHASE
		CgReport report = phase->getReport();
		printDOT(report.phaseName);
#endif

#if BENCHMARK_PHASES
		auto endTime = std::chrono::system_clock::now();
		double calculationTime = (endTime-startTime).count()/1e6;
		std::cout << "\t- " << "calculation took " << calculationTime << " sec" << std::endl;
#endif

		phases.pop();
	}

#if PRINT_FINAL_DOT
	printDOT("final");
#endif
}

void CallgraphManager::printDOT(std::string prefix) {

	std::string filename = prefix + "-" + "callgraph.dot";
	std::ofstream outfile(filename, std::ofstream::out);

	outfile << "digraph callgraph {\nnode [shape=oval]\n";

	for (auto node : graph) {

		std::string functionName = node->getFunctionName();
		std::string attributes;

		if (node->hasUniqueCallPath()) {
			attributes += "color=blue, ";
		}
		if (CgHelper::isConjunction(node)) {
			attributes += "color=green, ";
		}
		if (node->isInstrumented()) {
			attributes += "shape=doublecircle, ";
		}
		if (node->isUnwound()) {
			attributes += "shape=doubleoctagon, ";
		} else if (node->isLeafNode()) {
			attributes += "shape=octagon, ";
		}
		// runtime & expectedSamples in node label
		outfile << "\"" << functionName << "\"[" << attributes
				<< "label=\"" << functionName << "\\n"
				<< node->getRuntimeInSeconds() << "s" << "\\n"
				<< "#s: " << node->getExpectedNumberOfSamples()
				<< "\"]" << std::endl;
	}

	for (auto node : graph) {
		node->dumpToDot(outfile);
	}
	outfile << "\n}" << std::endl;
	outfile.close();

	std::cout << "DOT file dumped (" << filename << ")." << std::endl;

}
