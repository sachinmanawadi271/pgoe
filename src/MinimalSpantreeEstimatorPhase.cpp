
#include "MinimalSpantreeEstimatorPhase.h"

MinimalSpantreeEstimatorPhase::MinimalSpantreeEstimatorPhase() :
		EstimatorPhase("MinimalSpantree"),
		numberOfSkippedEdges(0),
		errorsFound(0) {
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

	while(!pq.empty()) {

		// try to insert edge with highest call count into span tree
		auto edge = pq.top();
		pq.pop();

		if (!CgHelper::canReachSameConjunction(edge.child, edge.parent)) {

			edge.child->addSpantreeParent(edge.parent);
		} else {

			numberOfSkippedEdges++;
			continue;
		}
	}

	builtinSanityCheck();
}

void MinimalSpantreeEstimatorPhase::builtinSanityCheck() {

	for (auto node : (*graph)) {

		if (!CgHelper::isConjunction(node)) {
			continue;
		}

		errorsFound += checkParentsForOverlappingCallpaths(node);
	}
}

int MinimalSpantreeEstimatorPhase::checkParentsForOverlappingCallpaths(CgNodePtr conjunctionNode) {

	int numberOfErrors = 0;
	std::map<CgNodePtr, SpantreeEdgeSet> paths;

	for (auto parentNode : conjunctionNode->getParentNodes()) {
		SpantreeEdgeSet path = getInstrumentationPathEdges(parentNode, conjunctionNode);
		paths[parentNode] = path;
	}

	for (auto pair : paths) {
		for (auto otherPair : paths) {

			if (pair == otherPair) {
				continue;
			}

			auto a = pair.second;
			auto b = otherPair.second;
			SpantreeEdgeSet intersect;

			std::set_intersection(
					a.begin(),a.end(),
					b.begin(),b.end(),
					std::inserter(intersect, intersect.begin()));

			if (!intersect.empty()) {

				std::cout << "ERROR in conjunction: " << *conjunctionNode << std::endl;
				std::cout << "    " << "Paths of " << *(pair.first) << " and " << *(otherPair.first)
						<< " intersect!" << std::endl;

				///XXX RN: do we need that additional output?
//				std::cout << "\t[";
//				for (auto e : a) {
//					std::cout << e << " | ";
//				}
//				std::cout << "]" << std::endl;
//				std::cout << "\t[";
//				for (auto e : b) {
//					std::cout << e << " | ";
//				}
//				std::cout << "]" << std::endl;

				numberOfErrors++;
			}
		}
	}

	return numberOfErrors;
}

SpantreeEdgeSet MinimalSpantreeEstimatorPhase::getInstrumentationPathEdges(CgNodePtr startNode,
		CgNodePtr childOfStartNode) {

	SpantreeEdge startEdge = SpantreeEdge( { childOfStartNode->getNumberOfCalls(startNode),
		childOfStartNode, startNode });

	SpantreeEdgeSet visitedEdges;
	std::queue<SpantreeEdge> workQueue;
	workQueue.push(startEdge);

	while (!workQueue.empty()) {

		auto edge = workQueue.front();
		workQueue.pop();

		visitedEdges.insert(edge);

		if (!edge.child->isSpantreeParent(edge.parent)) {
			continue;	// this edge is already instrumented
		}

		if (edge.parent->isRootNode()) {
			// add the implicit edge for the main function once it is reached
			SpantreeEdge implicitRootEdge = SpantreeEdge( {0, edge.parent, edge.parent} );
			visitedEdges.insert(implicitRootEdge);
		}

		for (auto grandParent : edge.parent->getParentNodes()) {
			SpantreeEdge grandParentEdge = SpantreeEdge( { edge.parent->getNumberOfCalls(grandParent),
					edge.parent, grandParent} );
			if (visitedEdges.find(grandParentEdge) == visitedEdges.end()) {
				workQueue.push(grandParentEdge);
			}
		}
	}

	return visitedEdges;
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

	std::cout << "\t" << "built-in sanity check done with " << errorsFound << " error(s)." << std::endl;
}


void MinimalSpantreeEstimatorPhase::printReport() {
	// only print the additional report
	printAdditionalReport();
}
