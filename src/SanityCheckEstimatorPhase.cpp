#include "SanityCheckEstimatorPhase.h"

SanityCheckEstimatorPhase::SanityCheckEstimatorPhase() :
		EstimatorPhase("SanityCheck", true),
		numberOfErrors(0){
}

SanityCheckEstimatorPhase::~SanityCheckEstimatorPhase() {}

void SanityCheckEstimatorPhase::modifyGraph(CgNodePtr mainMethod) {

	for (auto node : (*graph)) {

		// unwound nodes are fine as they are
		if(!CgHelper::isConjunction(node) || node->isInstrumentedConjunction()) {
			continue;
		}

		if (node->isUnwound()) {
			if (CgHelper::isOnCycle(node)) {
				std::cerr << "ERROR in unwound function is on a circle: "
						<< node->getFunctionName() << std::endl;
			} else {
				continue;
			}
		}

		numberOfErrors += CgHelper::uniqueInstrumentationTest(node);
	}

	// XXX idea: check that there is no instrumentation below unwound nodes
}

void SanityCheckEstimatorPhase::printAdditionalReport() {
	std::cout << "\t" << "SanityCheck done with "
			<< numberOfErrors << " errors." << std::endl;
}

