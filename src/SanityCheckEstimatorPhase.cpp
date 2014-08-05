#include "SanityCheckEstimatorPhase.h"

SanityCheckEstimatorPhase::SanityCheckEstimatorPhase() :
		EstimatorPhase("SanityCheck"),
		numberOfErrors(0){
}

SanityCheckEstimatorPhase::~SanityCheckEstimatorPhase() {}

void SanityCheckEstimatorPhase::modifyGraph(CgNodePtr mainMethod) {


	for (auto node : (*graph)) {

		// unwound nodes are fine as they are
		if(!CgHelper::isConjunction(node) || node->isUnwound()) {
			continue;
		}

		CgNodePtr oneUninstrumentedPath = NULL;
		CgNodePtrSet instrumentedPaths;

		// all parents' call paths BUT ONE have to be instrumented
		for (auto parentNode : node->getParentNodes()) {

			auto instrumentedNode = CgHelper::getInstrumentedNodeOnPath(parentNode);
			if(instrumentedNode == NULL || instrumentedNode->isSameFunction(mainMethod)) {

				if(oneUninstrumentedPath) {
					numberOfErrors++;
					std::cerr << "ERROR: Inconsistency in conjunction node: " << *node << std::endl
							<< "  paths of " << *parentNode << " and " << *oneUninstrumentedPath
							<< " not instrumented" << std::endl;
				} else {
					oneUninstrumentedPath = parentNode;
				}

			} else {
				// check that the instrumented paths all have a unique instrumentation marker
				if (instrumentedPaths.find(instrumentedNode) == instrumentedPaths.end()) {
					instrumentedPaths.insert(instrumentedNode);
				} else {
					numberOfErrors++;
					std::cerr << "ERROR: Inconsistency in conjunction node: " << *node << std::endl
							<< "  " << *parentNode << " call path not instrumented with unique marker."
							<< " (marker at: " << *instrumentedNode << ")" << std::endl;
				}
			}
		}
		// check that the uninstrumented path is not reachable from another instrumented node
		if (oneUninstrumentedPath) {
			for (auto instrumentedPath : instrumentedPaths) {
				if (CgHelper::reachableFrom(instrumentedPath, oneUninstrumentedPath)) {
					numberOfErrors++;
					std::cerr << "ERROR: Inconsistency in conjunction node: " << *node << std::endl
							<< "  uninstrumented parent "<< *oneUninstrumentedPath
							<< " is reachable from instrumented node " << *instrumentedPath << std::endl;
				}
			}
		}
	}

	// XXX idea: check that there is no instrumentation below unwound nodes
}

void SanityCheckEstimatorPhase::printReport() {
	// only print the additional report as it does not touch the graph
	printAdditionalReport();
}

void SanityCheckEstimatorPhase::printAdditionalReport() {
	std::cout << "==" << report.phaseName << "== Phase Report " << std::endl;
	std::cout << "\t" << "SanityCheck done with "
			<< numberOfErrors << " errors." << std::endl;
}

