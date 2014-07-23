
#include "MinimalSpantreeEstimatorPhase.h"

MinimalSpantreeEstimatorPhase::MinimalSpantreeEstimatorPhase() :
		EstimatorPhase("MinimalSpantree"),
		numberOfSkippedEdges(0) {
}

MinimalSpantreeEstimatorPhase::~MinimalSpantreeEstimatorPhase() {
}

void MinimalSpantreeEstimatorPhase::modifyGraph(CgNodePtr mainMethod) {

	std::priority_queue<SpantreeEdge, std::vector<SpantreeEdge>, MoreCalls> pq;
	// get all edges
	for (auto parentNode : (*graph)) {
		for (auto childNode : parentNode->getChildNodes()) {
			pq.push(SpantreeEdge({
				childNode->getNumberOfCalls(parentNode), childNode, parentNode
			}));
		}
	}

	CgNodePtrSet visitedNodes;
	visitedNodes.insert(mainMethod);

	while(!pq.empty()) {

		// try to insert edge with highest call count into span tree
		auto edge = pq.top();
		pq.pop();

		if (!CgHelper::isConnectedOnSpantree(edge.child, edge.parent)) {

			visitedNodes.insert(edge.child);
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

	unsigned long long numberOfInstrumentedCalls = 0;
	unsigned long long instrumentationOverhead = 0;

	for (auto childNode : (*graph)) {

		for (auto parentNode : childNode->getParentNodes()) {
			if(childNode->isSpantreeParent(parentNode)) {
				continue;
			} else {

				unsigned long long numberOfCalls = childNode->getNumberOfCalls(parentNode);
				numberOfInstrumentedCalls += numberOfCalls;
				instrumentationOverhead += (numberOfCalls * CgConfig::nanosPerInstrumentedCall);
			}
		}
	}

	std::cout << "\t" << numberOfSkippedEdges <<
			" edge(s) not part of Spanning Tree" << std::endl;
	std::cout << "\tinstrumentedCalls: " << numberOfInstrumentedCalls
			<< " | instrumentationOverhead: " << instrumentationOverhead << " ns" << std::endl
			<< "\t" << "overallOverhead: " << instrumentationOverhead << " ns"
			<< " | that is: " << instrumentationOverhead/1e9 <<" s"<< std::endl;

}


void MinimalSpantreeEstimatorPhase::printReport() {
	// only print the additional report
	printAdditionalReport();
}
