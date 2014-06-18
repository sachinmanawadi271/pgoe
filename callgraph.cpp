#include "callgraph.h"

#define VERBOSE 0
#define DEBUG 0

Callgraph::Callgraph(){
}

int Callgraph::putFunction(std::string fullQualifiedNameCaller, std::string fullQualifiedNameCallee){

	std::shared_ptr<CgNode> caller;
	std::shared_ptr<CgNode> callee;

	int returnCode = 0;

#if DEBUG > 1
	std::cout << "Putting pair (caller, callee) : (" << fullQualifiedNameCaller << ", " << fullQualifiedNameCallee << ") into graph." << std::endl;
#endif

	if(graph.find(fullQualifiedNameCaller) != graph.end()){
		// A node representing the caller already exists
		caller = graph.find(fullQualifiedNameCaller)->second;
#if DEBUG > 1
	std::cout << "fullQualifiedNameCaller (" << fullQualifiedNameCaller << ") already exists in call graph" << std::endl;
#endif
	} else {
		// Create a new node representing the caller
		caller = std::make_shared<CgNode>(fullQualifiedNameCaller);
		graph.insert(std::pair<std::string, std::shared_ptr<CgNode> >(fullQualifiedNameCaller, caller));
		returnCode++;
#if DEBUG > 1
	std::cout << "fullQualifiedNameCaller (" << fullQualifiedNameCaller << ") newly added to call graph" << std::endl;
#endif
	}

	if(graph.find(fullQualifiedNameCallee) != graph.end()){
		// A node representing the callee already exists
		callee = graph.find(fullQualifiedNameCallee)->second;
#if DEBUG > 1
	std::cout << "fullQualifiedNameCallee (" << fullQualifiedNameCallee << ") already exists in call graph" << std::endl;
#endif
	} else {
		// Create a new node representing the callee
		callee = std::make_shared<CgNode>(fullQualifiedNameCallee);
		graph.insert(std::pair<std::string, std::shared_ptr<CgNode> >(fullQualifiedNameCallee, callee));
		returnCode++;
#if DEBUG > 1
	std::cout << "fullQualifiedNameCallee (" << fullQualifiedNameCallee << ") newly added to call graph" << std::endl;
#endif
	}
#if VERBOSE > 2
	std::cout << "Caller: " << fullQualifiedNameCaller << caller.get() << "\nCalee: " << fullQualifiedNameCallee << callee.get() << std::endl;
#endif
	caller->addCallsNode(callee);
	callee->addIsCalledByNode(caller);
	return returnCode;
}

int Callgraph::putFunction(std::string fullQualifiedNameCaller, std::string filenameCaller, int lineCaller, std::string fullQualifiedNameCallee, int calls){
	putFunction(fullQualifiedNameCaller, fullQualifiedNameCallee);

	auto caller = findNode(fullQualifiedNameCaller);
	if(caller == NULL)
		std::cerr << "ERROR in looking up node." << std::endl;

	caller->setFilename(filenameCaller);
	caller->setLineNumber(lineCaller);

	auto callee = findNode(fullQualifiedNameCallee);
	callee->addNumberOfCalls(calls, caller);

	return 0;
}

std::shared_ptr<CgNode> Callgraph::findNode(std::string functionName){
	
	for (auto node : graph){
		auto fName = node.second->getFunctionName();
		if(fName.find(functionName) != std::string::npos)
			return node.second;
	}

	return NULL;

}

/*
 * This is the first and very basic implementation, only selecting parents of
 * conjunction nodes for hook placement.
 */
std::vector<std::shared_ptr<CgNode> > Callgraph::getNodesToMark(){
	std::vector<std::shared_ptr<CgNode> > nodesToMark;
	for (auto gNode : graph){
		if(gNode.second->getNeedsInstrumentation())
			nodesToMark.push_back(gNode.second);
	}

	return nodesToMark;
}

int Callgraph::markNodes(){

	int numberOfMarkedNodes = 0;
	std::queue<std::shared_ptr<CgNode> > workQueue;
	std::vector<CgNode*> done;

	workQueue.push(findMain());
	while(! workQueue.empty()){
#if DEBUG > 1
		std::cerr << "Queue Size: " << workQueue.size() << std::endl;
#endif
		auto node = workQueue.front();
		done.push_back(node.get());
		workQueue.pop();
		if(node == NULL){
			std::cerr << "node was NULL" << std::endl;
		}
		if(node->getCallers().size() > 1){
#if DEBUG > 1
		std::cout << "For node: " << node->getFunctionName() << " callers.size() = " << node->getCallers().size() << std::endl;
#endif
			for (auto nodeToInsert : node->getCallers()){
					nodeToInsert->setNeedsInstrumentation(true);
					numberOfMarkedNodes++;
			}

		}
		for (auto n : node->getCallees()){
			bool insert = true;
			for (auto refNode : done)
				if(refNode == n.get())
					insert = false;
			if(insert)
				workQueue.push(n);
		}
	}
	
	return numberOfMarkedNodes;
}


/*
 * While possible - Move hooks upwards along a call chain
 * return how many hooks have been re-placed
 */
int Callgraph::moveHooksUpwards(){
	int hooksHaveBeenMoved = 0;
	for (auto graphPair : graph){
		// If the node was not selected previously, we continue
		if(! graphPair.second->getNeedsInstrumentation())
			continue;

		// If it was selected, we try to move the hook upwards
		auto cur = graphPair.second;
		bool hasMoved = false;
		while(cur->getCallers().size() == 1){
			if((*cur->getCallers().begin())->getCallees().size() > 1)
				break;

			cur = *cur->getCallers().begin(); // This should be safe...
			hasMoved = true;
		}
		if(hasMoved)
			hooksHaveBeenMoved += 1;

		graphPair.second->setNeedsInstrumentation(false);
		cur->setNeedsInstrumentation(true);
		hasMoved = false;
	}

	return hooksHaveBeenMoved;
}

int Callgraph::getSize(){
	return graph.size();
}

void Callgraph::print(){

	auto mainNode = findMain();
	mainNode->print();

	std::cout << std::endl;
}

std::shared_ptr<CgNode> Callgraph::findMain(){

//	return findNode("main");

	for (auto node : graph){
		auto fName = node.second->getFunctionName();
//		std::cout << "Function: " << fName << std::endl;
		if(fName.find("main") != std::string::npos)
			return node.second;
	}

	return NULL;
}

void Callgraph::printDOT(std::string prefix){

	std::ofstream outfile(prefix + "-" + "callgraph.dot", std::ofstream::out);

	outfile << "digraph callgraph {\nnode [shape=oval]\n";

	for (auto mapPair : graph) {
		if(mapPair.second->hasUniqueParents()) {
			outfile << "\"" <<  mapPair.second->getFunctionName() << "\"[color=blue]" << std::endl;
		}
		if(mapPair.second->getNeedsInstrumentation()) {
			outfile << "\"" <<  mapPair.second->getFunctionName() << "\"[shape=doublecircle]" << std::endl;
		}
	}

	for (auto mapPair : graph) {
		mapPair.second->dumpToDot(outfile);
	}
	outfile << "\n}" << std::endl;
	outfile.close();

	std::cout << "DOT file dumped (prefix: " << prefix << ")."<< std::endl;

}
