#include "CallgraphManager.h"

CallgraphManager::CallgraphManager(Config* config) : config(config) {
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
void CallgraphManager::putNumberOfSamples(std::string name, unsigned long long numberOfSamples) {
	if (graphMapping.find(name) != graphMapping.end()) {
		graphMapping.find(name)->second->setExpectedNumberOfSamples(numberOfSamples);
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

	auto parentNode = graph.findNode(parentName);
	if (parentNode == nullptr) {
		std::cerr << "ERROR in looking up node." << std::endl;
	}
	parentNode->setFilename(parentFilename);
	parentNode->setLineNumber(parentLine);

	auto childNode = graph.findNode(childName);
	childNode->addCallData(parentNode, numberOfCalls, timeInSeconds);
}

void CallgraphManager::registerEstimatorPhase(EstimatorPhase* phase, bool noReport) {
	phases.push(phase);
	phase->injectConfig(config);
	phase->setGraph(&graph);

	if (noReport) {
		phase->setNoReport();
	}
}

void CallgraphManager::finalizeGraph() {
	graphMapping.clear();

	// also update all node attributes
	for (auto node : graph) {

		if (config->samplesFile.empty()) {
			node->updateNodeAttributes();
		} else {
			node->updateNodeAttributes(false);
		}

		CgNodePtrSet markerPositions = CgHelper::getPotentialMarkerPositions(node);
		node->getMarkerPositions().insert(markerPositions.begin(), markerPositions.end());

		std::for_each(markerPositions.begin(), markerPositions.end(),
				[&node](const CgNodePtr& markerPosition) { markerPosition->getDependentConjunctions().insert(node); });
	}
}

void CallgraphManager::thatOneLargeMethod() {

	finalizeGraph();
	auto mainFunction = graph.findMain();

	if (mainFunction == nullptr) {
		std::cerr << "CallgraphManager: Cannot find main function." << std::endl;
		exit(1);
	}

	while(!phases.empty()) {
		EstimatorPhase* phase = phases.front();

#if BENCHMARK_PHASES
		auto startTime = std::chrono::system_clock::now();
#endif
		phase->modifyGraph(mainFunction);
		phase->generateReport();

		phase->printReport();

		CgReport report = phase->getReport();
#if PRINT_DOT_AFTER_EVERY_PHASE
		printDOT(report.phaseName);
#endif	// PRINT_DOT_AFTER_EVERY_PHASE
#if DUMP_INSTRUMENTED_NAMES
		dumpInstrumentedNames(report);
#endif	// DUMP_INSTRUMENTED_NAMES
#if DUMP_UNWOUND_NAMES
		dumpUnwoundNames(report);
#endif	// DUMP_UNWOUND_NAMES

#if BENCHMARK_PHASES
		auto endTime = std::chrono::system_clock::now();
		double calculationTime = (endTime-startTime).count()/1e6;
		std::cout << "\t- " << "calculation took " << calculationTime << " sec" << std::endl;
#endif

		phases.pop();
	}

	std::cout << " ---- " << "Fastest Phase: " << std::setw(8) <<  config->fastestPhaseOvPercent << " % with "
			<< config->fastestPhaseName << std::endl;

#if PRINT_FINAL_DOT
	printDOT("final");
#endif
}

void CallgraphManager::printDOT(std::string prefix) {

	std::string filename = "out/callgraph-" + config->appName + "-" + prefix + ".dot";
	std::ofstream outfile(filename, std::ofstream::out);

	outfile << "digraph callgraph {\nnode [shape=oval]\n";

	unsigned long long callsForThreePercentOfOverhead = config->fastestPhaseOvSeconds * 10e9 * 0.03 / (double) CgConfig::nanosPerInstrumentedCall;

	for (auto node : graph) {

		std::string functionName = node->getFunctionName();
		std::string attributes;
		std::string additionalLabel;

		if (node->hasUniqueCallPath()) {
			attributes += "color=blue, ";
		}
		if (CgHelper::isConjunction(node)) {
			attributes += "color=green, ";
		}
		if (node->isInstrumentedWitness()) {
			attributes += "style=filled, ";

			if (node->getNumberOfCalls() > callsForThreePercentOfOverhead) {
				attributes += "fillcolor=red, ";
			} else {
				attributes += "fillcolor=grey, ";
			}
		}
		if (node->isInstrumentedConjunction()) {
			attributes += "style=filled, ";
			attributes += "fillcolor=palegreen, ";
		}
		if (node->isUnwound()) {
			attributes += "shape=doubleoctagon, ";
			additionalLabel += std::string("\\n unwindSteps: ");
			additionalLabel += std::to_string(node->getNumberOfUnwindSteps());
		} else if (node->isLeafNode()) {
			attributes += "shape=octagon, ";
		}

		additionalLabel += std::string("\\n #calls: ");
		additionalLabel += std::to_string(node->getNumberOfCalls());

		// runtime & expectedSamples in node label
		outfile << "\"" << functionName << "\"[" << attributes
				<< "label=\"" << functionName << "\\n"
				<< node->getRuntimeInSeconds() << "s" << "\\n"
				<< "#samples: " << node->getExpectedNumberOfSamples()
				<< additionalLabel << "\"]" << std::endl;
	}

	for (auto node : graph) {
		node->dumpToDot(outfile);
	}
	outfile << "\n}" << std::endl;
	outfile.close();

//	std::cout << "DOT file dumped (" << filename << ")." << std::endl;

}

void CallgraphManager::dumpInstrumentedNames(CgReport report) {
	std::string filename = "out/instrumented-" + config->appName + "-" + report.phaseName + ".txt";
    std::size_t found = filename.find("out/instrumented-"+config->appName+"-"+"Incl");
	if (found!=std::string::npos) {
        filename = "out/instrumented-" + config->appName+".txt";
    }
	std::ofstream outfile(filename, std::ofstream::out);

	if (report.instrumentedNodes.empty()) {
		outfile << "aFunctionThatDoesNotExist" << std::endl;
	} else {
		for (auto name : report.instrumentedNames) {
			outfile << name << std::endl;
		}
	}

}

void CallgraphManager::dumpUnwoundNames(CgReport report) {
	std::string filename = "out/unw-" + config->appName + "-" + report.phaseName + ".txt";
	std::ofstream outfile(filename, std::ofstream::out);

	for (auto pair : report.unwoundNames) {
		std::string name = pair.first;
		int unwindSteps = pair.second;

		outfile << unwindSteps << " " << name << std::endl;
	}
}

std::map<std::string, CgNodePtr> CallgraphManager::getGraphMapping(CallgraphManager *cg){
	return cg->graphMapping;
};

Callgraph CallgraphManager::getCallgraph(CallgraphManager *cg){
    return cg->graph;
}


