#include "callgraph.h"

#define VERBOSE 0

Callgraph::Callgraph(){
}

int Callgraph::putFunction(std::string fullQualifiedNameCaller, std::string fullQualifiedNameCallee){

	std::shared_ptr<CgNode> caller;
	std::shared_ptr<CgNode> callee;

	int returnCode = 0;

	

	if(graph.find(fullQualifiedNameCaller) != graph.end()){
		// A node representing the caller already exists
		caller = graph.find(fullQualifiedNameCaller)->second;
	} else {
		// Create a new node representing the caller
		caller = std::make_shared<CgNode>(fullQualifiedNameCaller);
		graph.insert(std::pair<std::string, std::shared_ptr<CgNode> >(fullQualifiedNameCaller, caller));
		returnCode++;
	}

	if(graph.find(fullQualifiedNameCallee) != graph.end()){
		// A node representing the callee already exists
		callee = graph.find(fullQualifiedNameCallee)->second;
	} else {
		// Create a new node representing the callee
		callee = std::make_shared<CgNode>(fullQualifiedNameCallee);
		graph.insert(std::pair<std::string, std::shared_ptr<CgNode> >(fullQualifiedNameCallee, callee));
		returnCode++;
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

	auto callee = findNode(fullQualifiedNameCallee);
	callee->addNumberOfCalls(calls);

	auto caller = findNode(fullQualifiedNameCaller);
	if(caller == NULL)
		std::cerr << "ERROR in looking up node." << std::endl
;
	caller->setFilename(filenameCaller);
	caller->setLineNumber(lineCaller);
}

std::shared_ptr<CgNode> Callgraph::findNode(std::string functionName){
	
	for(auto node : graph){
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
	std::queue<std::shared_ptr<CgNode> > workQueue;
	std::vector<CgNode*> done;

	workQueue.push(findMain());
	while(! workQueue.empty()){
		std::cerr << "Queue Size: " << workQueue.size() << std::endl;
		auto node = workQueue.front();
		done.push_back(node.get());
		workQueue.pop();
		if(node == NULL){
			std::cerr << "node was NULL" << std::endl;
		}
		if(node->getCallers().size() > 1){
			bool insert = true;

			for(auto nodeToInsert : node->getCallers()){
				for(auto refNode : nodesToMark){
					if(refNode == nodeToInsert)
						insert = false;
				}
				if(insert){
					nodesToMark.push_back(nodeToInsert);
					nodeToInsert->setNeedsInstrumentation(true);
				}
			}

/*
			// JP: This is the wrong implementation which was used in the paper
			// JP: The problem is: that estimated both benchmarks quite good -> error in śelector?
			for(auto refNode : nodesToMark){
				if(refNode == node)
					insert = false;
			}
			if(insert)
				nodesToMark.push_back(node);
*/
		}
		for(auto n : node->getCallees()){
			bool insert = true;
			for(auto refNode : done)
				if(refNode == n.get())
					insert = false;
			if(insert)
				workQueue.push(n);
		}
	}
	
	return nodesToMark;

}

/*
 * While possible - Move hooks upwards along a call chain
 * return how many hooks have been re-placed
 */
int Callgraph::moveHooksUpwards(){
	int hooksHaveBeenMoved = 0;
	for(auto graphPair : graph){
		// If the node was not selected previously, we continue
		if(! graphPair.second->getNeedsInstrumentation())
			continue;

		// If it was selected, we try to move the hook upwards
		auto cur = graphPair.second;
		bool hasMoved = false;
		while(cur->getCallers().size() == 1){
			cur = cur->getCallers()[0]; // This should be safe...
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

	for(auto node : graph){
		auto fName = node.second->getFunctionName();
//		std::cout << "Function: " << fName << std::endl;
		if(fName.find("main") != std::string::npos)
			return node.second;
	}

	return NULL;
}

void Callgraph::printDOT(){

	std::ofstream outfile("callgraph.dot", std::ofstream::out);

	outfile << "digraph callgraph {\nnode [shape=oval]\n";

	for( auto mapPair : graph){
		mapPair.second->dumpToDot(outfile);
	}

	outfile << "\n}" << std::endl;
	outfile.close();
	std::cout << "DOT file dumped." << std::endl;

}
