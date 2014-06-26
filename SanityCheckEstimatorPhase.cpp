#include "SanityCheckEstimatorPhase.h"

SanityCheckEstimatorPhase::SanityCheckEstimatorPhase(
		std::map<std::string, std::shared_ptr<CgNode> >* graph) :
		EstimatorPhase(graph, "SanityCheck") {
}

SanityCheckEstimatorPhase::~SanityCheckEstimatorPhase() {}

void SanityCheckEstimatorPhase::modifyGraph(std::shared_ptr<CgNode> mainMethod) {

	// check that all conjunctions are either instrumented or unwound
	for (auto pair : (*graph)) {
		auto node = pair.second;

		if(CgHelper::isConjunction(node) && !node->isUnwound()) {

			// all parents' call paths have to be instrumented
			for (auto parentNode : node->getParentNodes()) {

				if(!CgHelper::isOnInstrumentedPath(parentNode)) {
					std::cerr << "ERROR: Inconsistency in conjunction node: " << node->getFunctionName() << std::endl
							<< "  path of : " << parentNode->getFunctionName() << " not instrumented" << std::endl;
				}
			}
		}
	}
}

