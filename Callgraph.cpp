#include "Callgraph.h"

#define VERBOSE 0
#define DEBUG 0

#define PRINT_DOT_AFTER_EVERY_PHASE 0

Callgraph::Callgraph(int samplesPerSecond) :
		samplesPerSecond(samplesPerSecond) {

	// XXX phases are always hardcoded and predetermined for now
	phases.push(new RemoveUnrelatedNodesEstimatorPhase(&graph));
//	phases.push(new MinimalSpantreeEstimatorPhase(&graph));	// XXX does not hinder other phases
	phases.push(new InstrumentEstimatorPhase(&graph));
	phases.push(new MoveInstrumentationUpwardsEstimatorPhase(&graph));
	phases.push(new DeleteOneInstrumentationEstimatorPhase(&graph));
//	phases.push(new UnwindEstimatorPhase(&graph)); 	// TODO deactivated
	phases.push(new SanityCheckEstimatorPhase(&graph));
}

int Callgraph::putFunction(std::string parentName, std::string childName) {

	std::shared_ptr<CgNode> parentNode;
	std::shared_ptr<CgNode> childNode;

	int returnCode = 0;

#if DEBUG > 1
	std::cout << "Putting pair (caller, callee) : (" << parentName << ", " << childName << ") into graph." << std::endl;
#endif

	if (graph.find(parentName) != graph.end()) {
		// A node representing the caller already exists
		parentNode = graph.find(parentName)->second;
#if DEBUG > 1
		std::cout << "fullQualifiedNameCaller (" << parentName << ") already exists in call graph" << std::endl;
#endif
	} else {
		// Create a new node representing the caller
		parentNode = std::make_shared<CgNode>(parentName);
		graph.insert(
				std::pair<std::string, std::shared_ptr<CgNode> >(parentName,
						parentNode));
		returnCode++;
#if DEBUG > 1
		std::cout << "fullQualifiedNameCaller (" << parentName << ") newly added to call graph" << std::endl;
#endif
	}

	if (graph.find(childName) != graph.end()) {
		// A node representing the callee already exists
		childNode = graph.find(childName)->second;
#if DEBUG > 1
		std::cout << "fullQualifiedNameCallee (" << childName << ") already exists in call graph" << std::endl;
#endif
	} else {
		// Create a new node representing the callee
		childNode = std::make_shared<CgNode>(childName);
		graph.insert(
				std::pair<std::string, std::shared_ptr<CgNode> >(childName,
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

std::shared_ptr<CgNode> Callgraph::findNode(std::string functionName) {

	for (auto node : graph) {
		auto fName = node.second->getFunctionName();
		if (fName.find(functionName) != std::string::npos) {
			return node.second;
		}
	}

	return NULL;

}

int helper(int i, std::pair< std::string, std::shared_ptr<CgNode> > pair) {
	if(pair.second->isInstrumented()) {
		return i+1;
	}
	return i;
}

void Callgraph::thatOneLargeMethod() {

	updateNodeAttributes();

	while(!phases.empty()) {
		EstimatorPhase* phase = phases.front();

		phase->modifyGraph(findMain());
		phase->generateReport();

		phase->printReport();
//		CgReport report = phase->getReport();
#if PRINT_DOT_AFTER_EVERY_PHASE
		this->printDOT(report.phaseName);
#endif
		phases.pop();
	}

	// TODO more statistics, more abstraction
	// final statistics
	int instrumentedMethods =
			std::accumulate(
					graph.begin(),
					graph.end(),
					0,
					// RN: i always wanted to use a lambda function in c++ for once
					[] (int i, std::pair< std::string, std::shared_ptr<CgNode> > pair) {
						return pair.second->isInstrumented() ? i+1 : i;
					}
			);

	std::cout << "instrumented methods: " << instrumentedMethods << std::endl;
}

void Callgraph::updateNodeAttributes() {
	for (auto pair : graph) {
		pair.second->updateNodeAttributes(this->samplesPerSecond);
	}
}

int Callgraph::getSize() {
	return graph.size();
}

void Callgraph::print() {

	auto mainNode = findMain();
	mainNode->print();

	std::cout << std::endl;
}

std::shared_ptr<CgNode> Callgraph::findMain() {

	for (auto node : graph) {
		auto fName = node.first;

		if (fName.find("main") == 0) {	// starting with "main"
			return node.second;
		}
	}

	return NULL;
}

void Callgraph::printDOT(std::string prefix) {

	std::string filename = prefix + "-" + "callgraph.dot";
	std::ofstream outfile(filename, std::ofstream::out);

	outfile << "digraph callgraph {\nnode [shape=oval]\n";

	for (auto mapPair : graph) {

		auto node = mapPair.second;
		std::string functionName = node->getFunctionName();

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

	for (auto mapPair : graph) {
		mapPair.second->dumpToDot(outfile);
	}
	outfile << "\n}" << std::endl;
	outfile.close();

	std::cout << "DOT file dumped (" << filename << ")." << std::endl;

}
