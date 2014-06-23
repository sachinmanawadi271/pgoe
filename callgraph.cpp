#include "callgraph.h"

#define VERBOSE 0
#define DEBUG 0

Callgraph::Callgraph(int samplesPerSecond) :
		samplesPerSecond(samplesPerSecond) {
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

/*
 * This is the first and very basic implementation, only selecting parents of
 * conjunction nodes for hook placement.
 */
std::vector<std::shared_ptr<CgNode> > Callgraph::getNodesRequiringInstrumentation() {
	std::vector<std::shared_ptr<CgNode> > nodesToMark;
	for (auto gNode : graph) {
		if (gNode.second->getNeedsInstrumentation()) {
			nodesToMark.push_back(gNode.second);
		}
	}

	return nodesToMark;
}

int Callgraph::markNodesRequiringInstrumentation() {

	updateNodeAttributes();

	int numberOfMarkedNodes = 0;
	std::queue<std::shared_ptr<CgNode> > workQueue;
	std::vector<CgNode*> done;

	workQueue.push(findMain());
	while (!workQueue.empty()) {
#if DEBUG > 1
		std::cerr << "Queue Size: " << workQueue.size() << std::endl;
#endif
		auto node = workQueue.front();
		done.push_back(node.get());
		workQueue.pop();
		if (node == NULL) {
			std::cerr << "node was NULL" << std::endl;
		}
		if (node->getParentNodes().size() > 1) {
#if DEBUG > 1
			std::cout << "For node: " << node->getFunctionName() << " callers.size() = " << node->getParentNodes().size() << std::endl;
#endif
			for (auto nodeToInsert : node->getParentNodes()) {
				nodeToInsert->setNeedsInstrumentation(true);
				numberOfMarkedNodes++;
			}

		}
		for (auto n : node->getChildNodes()) {
			bool insert = true;
			for (auto refNode : done) {
				if (refNode == n.get()) {
					insert = false;
				}
			}
			if (insert) {
				workQueue.push(n);
			}
		}
	}

	return numberOfMarkedNodes;
}

void Callgraph::updateNodeAttributes() {
	for (auto pair : graph) {
		pair.second->updateNodeAttributes(this->samplesPerSecond);
	}
}

/*
 * While possible - Move hooks upwards along a call chain
 * return how many hooks have been re-placed
 */
int Callgraph::moveHooksUpwards() {
	int hooksHaveBeenMoved = 0;
	for (auto graphPair : graph) {
		// If the node was not selected previously, we continue
		if (!graphPair.second->getNeedsInstrumentation()) {
			continue;
		}

		// If it was selected, we try to move the hook upwards
		auto cur = graphPair.second;
		bool hasMoved = false;
		while (cur->getParentNodes().size() == 1) {
			if ((*cur->getParentNodes().begin())->getChildNodes().size() > 1) {
				break;
			}

			cur = *cur->getParentNodes().begin(); // This should be safe...
			hasMoved = true;
		}
		if (hasMoved) {
			hooksHaveBeenMoved += 1;
		}
		graphPair.second->setNeedsInstrumentation(false);
		cur->setNeedsInstrumentation(true);
		hasMoved = false;
	}

	return hooksHaveBeenMoved;
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
		auto fName = node.second->getFunctionName();

		if (fName.find("main") != std::string::npos) {
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
		if (node->getNeedsInstrumentation()) {
			outfile << "\"" << functionName << "\"[shape=doublecircle]"
					<< std::endl;
		}
		if (node->isLeafNode()) {
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
