
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
	// check if all nodes are contained in span tree yet
	while(!pq.empty()) {

		// try to insert edge with highest call count into span tree
		auto edge = pq.top();
		pq.pop();

		if (visitedNodes.find(edge.child) != visitedNodes.end()) {
			numberOfSkippedEdges++;
			continue;
		} else {
			visitedNodes.insert(edge.child);
			edge.child->setSpantreeParent(edge.parent);
		}

	}
}

void MinimalSpantreeEstimatorPhase::printAdditionalReport() {
	std::cout << "==" << report.phaseName << "== Phase Report " << std::endl;

	// TODO this number should be in a config file
	const int nanosPerInstrumentedCall = 4;

	unsigned long long numberOfInstrumentedCalls;
	unsigned long long instrumentationOverhead;

	for (auto pair : (*graph)) {
		auto childNode = pair.second;

		for (auto parentNode : childNode->getParentNodes()) {
			if(childNode->isSpantreeParent(parentNode)) {
				continue;
			} else {
				unsigned long long numberOfCalls = childNode->getNumberOfCalls(parentNode);
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
