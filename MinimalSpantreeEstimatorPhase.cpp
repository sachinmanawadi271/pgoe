
#include "MinimalSpantreeEstimatorPhase.h"

MinimalSpantreeEstimatorPhase::MinimalSpantreeEstimatorPhase(
		std::map<std::string, std::shared_ptr<CgNode> >* graph) :
		EstimatorPhase(graph, "MinimalSpantree"){
}

MinimalSpantreeEstimatorPhase::~MinimalSpantreeEstimatorPhase() {
}

void MinimalSpantreeEstimatorPhase::modifyGraph(std::shared_ptr<CgNode> mainMethod) {

	std::priority_queue<SpantreeEdge, std::vector<SpantreeEdge>, MoreCalls> pq;
	// get all edges
	for (auto pair : (*graph)) {
		auto parentNode = pair.second;
		for (auto childNode : parentNode->getChildNodes()) {
			pq.push(SpantreeEdge({
				(int) childNode->getNumberOfCalls(parentNode),
				childNode,
				parentNode
			}));
		}
	}

	std::set<std::shared_ptr<CgNode> > visitedNodes;
	visitedNodes.insert(mainMethod);

	// check if all nodes are contained in span tree yet
	while(!pq.empty()) {

		// try to insert edge with highest call count into span tree
		auto edge = pq.top();
		pq.pop();

		if (visitedNodes.find(edge.child) != visitedNodes.end()) {
			continue;
		} else {
			visitedNodes.insert(edge.child);
			edge.child->setSpantreeParent(edge.parent);
		}

	}
}

void MinimalSpantreeEstimatorPhase::printAdditionalReport() {
	std::cout << "==" << report.phaseName << "== Phase Report " << std::endl;
	std::cout << ">>\t" << "NOT IMPLEMENTED YET!!" << std::endl;
}


void MinimalSpantreeEstimatorPhase::printReport() {
	// only print the additional report
	printAdditionalReport();
}
