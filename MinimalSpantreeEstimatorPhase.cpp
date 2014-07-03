
#include "MinimalSpantreeEstimatorPhase.h"

MinimalSpantreeEstimatorPhase::MinimalSpantreeEstimatorPhase(
		std::map<std::string, std::shared_ptr<CgNode> >* graph) :
		EstimatorPhase(graph, "MinimalSpantree"),
		numberOfSkippedEdges(0) {
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
				childNode->getNumberOfCalls(parentNode),
				childNode,
				parentNode
			}));
		}
	}

	std::set<std::shared_ptr<CgNode> > visitedNodes;
	visitedNodes.insert(mainMethod);

	while(!pq.empty()) {

		// try to insert edge with highest call count into span tree
		auto edge = pq.top();
		pq.pop();

		// XXX
		std::cout << "#" << edge.calls
				<< "\t" << edge.parent->getFunctionName()
				<< "  ->\t" << edge.child->getFunctionName() << std::endl;
		for(auto v : visitedNodes) {
			std::cout << v->getFunctionName() << ", ";
		}
		std::cout << std::endl;

		bool childVisited = visitedNodes.find(edge.child) != visitedNodes.end();
		bool parentVisited = visitedNodes.find(edge.parent) != visitedNodes.end();

		if (!childVisited) {
			visitedNodes.insert(edge.child);
			edge.child->addSpantreeParent(edge.parent);

		} else if(childVisited && !parentVisited) {
			visitedNodes.insert(edge.parent);
			edge.child->addSpantreeParent(edge.parent);
		} else {
			numberOfSkippedEdges++;
			continue;
		}

	}
}

void MinimalSpantreeEstimatorPhase::printAdditionalReport() {
	std::cout << "==" << report.phaseName << "== Phase Report " << std::endl;

	// TODO this number should be in a config file
	const int nanosPerInstrumentedCall = 4;

	unsigned long long numberOfInstrumentedCalls = 0;
	unsigned long long instrumentationOverhead = 0;

	for (auto pair : (*graph)) {
		auto childNode = pair.second;

		for (auto parentNode : childNode->getParentNodes()) {
			if(childNode->isSpantreeParent(parentNode)) {
				continue;
			} else {

				unsigned long long numberOfCalls;
				// special case, instrumentation at call site
				if (!CgHelper::isConjunction(childNode)) {
					// XXX numberOfCalls =
				}

				numberOfCalls = childNode->getNumberOfCalls(parentNode);
				numberOfInstrumentedCalls += numberOfCalls;
				instrumentationOverhead += (numberOfCalls * nanosPerInstrumentedCall);
			}
		}
	}

	std::cout << "\t" << numberOfSkippedEdges <<
			" edge(s) not part of Spanning Tree" << std::endl;
	std::cout << "\tinstrumentedCalls: " << numberOfInstrumentedCalls
			<< " | instrumentationOverhead: " << instrumentationOverhead << " ns" << std::endl
			<< "\t" << "that is: " << instrumentationOverhead/1e9 <<" s"<< std::endl;

}


void MinimalSpantreeEstimatorPhase::printReport() {
	// only print the additional report
	printAdditionalReport();
}
