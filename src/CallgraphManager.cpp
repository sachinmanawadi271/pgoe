#include "CallgraphManager.h"


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

void CallgraphManager::putNumberOfStatements(std::string name, int numberOfStatements) {
	CgNodePtr node = findOrCreateNode(name);
	node->setNumberOfStatements(numberOfStatements);
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
	if (parentNode == nullptr) {
		std::cerr << "ERROR in looking up node." << std::endl;
	}
	parentNode->setFilename(parentFilename);
	parentNode->setLineNumber(parentLine);

	auto childNode = findNode(childName);
	childNode->addCallData(parentNode, numberOfCalls, timeInSeconds);
}


CgNodePtr CallgraphManager::findMain() {
	if( findNode("main") ) {
		return findNode("main");
	} else {
		// simply search a method containing "main" somewhere
		for (auto node : graph) {
			auto fName = node->getFunctionName();
			if (fName.find("main") != fName.npos) {
				return node;
			}
		}
		return nullptr;
	}
}

CgNodePtr CallgraphManager::findNode(std::string functionName) {

	for (auto node : graph) {
		auto fName = node->getFunctionName();
		if (fName == functionName) {
			return node;
		}
	}
	return nullptr;
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
#if DUMP_INSTRUMENTED_NAMES
		dumpInstrumentedNames(report);
#endif	// DUMP_INSTRUMENTED_NAMES
#endif	// PRINT_DOT_AFTER_EVERY_PHASE

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

	std::string filename = "callgraph-" + prefix + ".dot";
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
		} else if (node->isInstrumented()) {
			attributes += "shape=doublecircle, ";
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

void CallgraphManager::dumpInstrumentedNames(CgReport report) {
	std::string filename = "instrumented-" + report.phaseName + ".txt";
	std::ofstream outfile(filename, std::ofstream::out);

	for (auto name : report.instrumentedNames) {
		outfile << name << std::endl;
	}
}
