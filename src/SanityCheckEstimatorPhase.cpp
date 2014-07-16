#include "SanityCheckEstimatorPhase.h"

SanityCheckEstimatorPhase::SanityCheckEstimatorPhase(
		std::map<std::string, CgNodePtr>* graph) :
		EstimatorPhase(graph, "SanityCheck"),
		numberOfErrors(0){
}

SanityCheckEstimatorPhase::~SanityCheckEstimatorPhase() {}

void SanityCheckEstimatorPhase::modifyGraph(CgNodePtr mainMethod) {

	// check that all conjunctions are either instrumented or unwound
	for (auto pair : (*graph)) {
		auto node = pair.second;

		if(!CgHelper::isConjunction(node) || node->isUnwound()) {
			continue;
		}

		CgNodePtr oneUninstrumentedPath = 0;
		std::set<CgNodePtr> instrumentedPaths;

		// all parents' call paths BUT ONE have to be instrumented
		for (auto parentNode : node->getParentNodes()) {


			if(CgHelper::getInstrumentedNodeOnPath(parentNode) == NULL) {
				if(oneUninstrumentedPath) {
					numberOfErrors++;
					std::cerr << "ERROR: Inconsistency in conjunction node: \"" << node->getFunctionName()
							<< "\"" << std::endl
							<< "  paths of \"" << parentNode->getFunctionName()
							<< "\" and \"" << oneUninstrumentedPath->getFunctionName()
							<< "\" not instrumented" << std::endl;
				} else {
					oneUninstrumentedPath = parentNode;
				}
			} else {
				instrumentedPaths.insert(parentNode);
			}
		}
		// check that the uninstrumented path is not reachable from another instrumented node
		if (oneUninstrumentedPath) {
			for (auto instrumentedPath : instrumentedPaths) {
				if (CgHelper::reachableFrom(instrumentedPath, oneUninstrumentedPath)) {
					numberOfErrors++;
					std::cerr << "ERROR: Inconsistency in conjunction node: \"" << node->getFunctionName()
							<< "\"" << std::endl
							<< "  uninstrumented parent \""<< oneUninstrumentedPath->getFunctionName()
							<< "\" is reachable from instrumented node \""
							<< instrumentedPath->getFunctionName() << "\"" << std::endl;
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

