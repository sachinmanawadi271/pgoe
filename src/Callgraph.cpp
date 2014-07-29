#include "Callgraph.h"

#include <chrono>	// easy time measurement

#define VERBOSE 0
#define DEBUG 0

#define BENCHMARK_PHASES 1
#define PRINT_DOT_AFTER_EVERY_PHASE 1

Callgraph::Callgraph(int samplesPerSecond) :
		samplesPerSecond(samplesPerSecond) {
}

int Callgraph::putFunction(std::string parentName, std::string childName) {

	CgNodePtr parentNode;
	CgNodePtr childNode;

	int returnCode = 0;

#if DEBUG > 1
	std::cout << "Putting pair (caller, callee) : (" << parentName << ", " << childName << ") into graph." << std::endl;
#endif

	if (graphMapping.find(parentName) != graphMapping.end()) {
		// A node representing the caller already exists
		parentNode = graphMapping.find(parentName)->second;
#if DEBUG > 1
		std::cout << "fullQualifiedNameCaller (" << parentName << ") already exists in call graph" << std::endl;
#endif
	} else {
		// Create a new node representing the caller
		parentNode = std::make_shared<CgNode>(parentName);
		graphMapping.insert(
				std::pair<std::string, CgNodePtr>(parentName,
						parentNode));
		returnCode++;
#if DEBUG > 1
		std::cout << "fullQualifiedNameCaller (" << parentName << ") newly added to call graph" << std::endl;
#endif
	}

	if (graphMapping.find(childName) != graphMapping.end()) {
		// A node representing the callee already exists
		childNode = graphMapping.find(childName)->second;
#if DEBUG > 1
		std::cout << "fullQualifiedNameCallee (" << childName << ") already exists in call graph" << std::endl;
#endif
	} else {
		// Create a new node representing the callee
		childNode = std::make_shared<CgNode>(childName);
		graphMapping.insert(
				std::pair<std::string, CgNodePtr>(childName,
						childNode));
		returnCode++;
#if DEBUG > 1
		std::cout << "fullQualifiedNameCallee (" << childName << ") newly added to call graph" << std::endl;
#endif
	}
#if VERBOSE > 2
	std::cout << "Caller: " << parentName << parentNode.get() << "\nCalee: " << childName << childNode.get() << std::endl;
#endif
	parentNode->addChildNode(childNode);
	childNode->addParentNode(parentNode);
	return returnCode;
}

int Callgraph::putFunction(std::string parentName, std::string parentFilename,
		int parentLine, std::string childName, unsigned long long numberOfCalls,
		double timeInSeconds) {

	putFunction(parentName, childName);

	auto parentNode = findNode(parentName);
	if (parentNode == NULL) {
		std::cerr << "ERROR in looking up node." << std::endl;
	}
	parentNode->setFilename(parentFilename);
	parentNode->setLineNumber(parentLine);

	auto childNode = findNode(childName);
	childNode->addCallData(parentNode, numberOfCalls, timeInSeconds);

	return 0;
}


CgNodePtr Callgraph::findMain() {
	return findNode("main");
}

CgNodePtr Callgraph::findNode(std::string functionName) {

	for (auto node : graphMapping) {
		auto fName = node.first;
		if (fName == functionName) {
			return node.second;
		}
	}
	return NULL;
}

void Callgraph::registerEstimatorPhase(EstimatorPhase* phase) {
	phases.push(phase);
	phase->setGraph(&graph);
}

void Callgraph::optimizeGraph() {
	// build the acceleration structure
	for (auto pair : graphMapping) {
		graph.insert(pair.second);
	}

	// also update all node attributes
	for (auto node : graph) {
		node->updateNodeAttributes(this->samplesPerSecond);
	}
}

void Callgraph::thatOneLargeMethod() {

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
		this->printDOT(report.phaseName);
#endif

#if BENCHMARK_PHASES
		auto endTime = std::chrono::system_clock::now();
		double calculationTime = (endTime-startTime).count()/1e6;
		std::cout << "\t- " << "calculation took " << calculationTime << " sec" << std::endl;
#endif

		phases.pop();
	}

	// TODO more statistics, more abstraction
	// final statistics
	int instrumentedMethods =
			std::accumulate(
					graphMapping.begin(),
					graphMapping.end(),
					0,
					// RN: i always wanted to use a lambda function in c++ for once
					[] (int i, std::pair< std::string, CgNodePtr> pair) {
						return pair.second->isInstrumented() ? i+1 : i;
					}
			);

	std::cout << "instrumented methods: " << instrumentedMethods << std::endl;
}

void Callgraph::printDOT(std::string prefix) {

	std::string filename = prefix + "-" + "callgraph.dot";
	std::ofstream outfile(filename, std::ofstream::out);

	outfile << "digraph callgraph {\nnode [shape=oval]\n";

	for (auto mapPair : graphMapping) {

		auto node = mapPair.second;
		std::string functionName = mapPair.first;

		if (node->hasUniqueCallPath()) {
			outfile << "\"" << functionName << "\"[color=blue]" << std::endl;
		}
		if (node->isInstrumented()) {
			outfile << "\"" << functionName << "\"[shape=doublecircle]"
					<< std::endl;
		}
		if (node->isUnwound()) {
			outfile << "\"" << functionName << "\"[shape=doubleoctagon]" << std::endl;
		} else if (node->isLeafNode()) {
			outfile << "\"" << functionName << "\"[shape=octagon]" << std::endl;
		}
		// runtime & expectedSamples in node label
		outfile << "\"" << functionName << "\"[label=\"" << functionName << "\\n"
				<< node->getRuntimeInSeconds() << "s" << "\\n"
				<< "#s: " << node->getExpectedNumberOfSamples()
				<< "\"]" << std::endl;
	}

	for (auto mapPair : graphMapping) {
		mapPair.second->dumpToDot(outfile);
	}
	outfile << "\n}" << std::endl;
	outfile.close();

	std::cout << "DOT file dumped (" << filename << ")." << std::endl;

}
