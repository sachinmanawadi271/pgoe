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

void Callgraph::erase(CgNodePtr node, bool rewireAfterDeletion) {
	if (CgHelper::isConjunction(node) && node->getChildNodes().size() > 1) {
		std::cerr << "Error: Cannot remove node with multiple parents AND multiple children." << std::endl;
		exit(1);
	}

	if (CgHelper::isConjunction(node)) {
		// XXX erasing a node with multiple parents invalidates edge weights (they are visible in the dot output)
	}

	for (auto parent : node->getParentNodes()) {
		parent->removeChildNode(node);
	}
	for (auto child : node->getChildNodes()) {
		child->removeParentNode(node);
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

	for (auto node : graphMapping) {
		auto fName = node.first;
		if (fName == functionName) {
			return node.second;
		}
	}
	return NULL;
}

void CallgraphManager::registerEstimatorPhase(EstimatorPhase* phase) {
	phases.push(phase);
	phase->setGraph(&graph);
}

void CallgraphManager::optimizeGraph() {
	// build the acceleration structure
	for (auto pair : graphMapping) {
		graph.insert(pair.second);
	}

	// also update all node attributes
	for (auto node : graph) {
		node->updateNodeAttributes(this->samplesPerSecond);
	}
}

void CallgraphManager::thatOneLargeMethod() {

	optimizeGraph();

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
