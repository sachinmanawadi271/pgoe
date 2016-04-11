#include "SanityCheckEstimatorPhase.h"

SanityCheckEstimatorPhase::SanityCheckEstimatorPhase() :
		EstimatorPhase("SanityCheck"),
		numberOfErrors(0){
}

SanityCheckEstimatorPhase::~SanityCheckEstimatorPhase() {}

void SanityCheckEstimatorPhase::modifyGraph(CgNodePtr mainMethod) {

	for (auto node : (*graph)) {

		// unwound nodes are fine as they are
		if(!CgHelper::isConjunction(node) || node->isInstrumentedConjunction()) {
			continue;
		}

		if (node->isUnwound() && CgHelper::isOnCycle(node)) {
			std::cerr << "ERROR in unwound function is on a circle: "
					<< node->getFunctionName() << std::endl;
		}

		numberOfErrors += CgHelper::uniqueInstrumentationTest(node);
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

