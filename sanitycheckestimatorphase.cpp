#include "sanitycheckestimatorphase.h"

SanityCheckEstimatorPhase::SanityCheckEstimatorPhase(
		std::map<std::string, std::shared_ptr<CgNode> >* graph) :
		EstimatorPhase(graph, "SanityCheck") {
}

SanityCheckEstimatorPhase::~SanityCheckEstimatorPhase() {}

void SanityCheckEstimatorPhase::modifyGraph(std::shared_ptr<CgNode> mainMethod) {
	for (auto pair : (*graph)) {
		auto node = pair.second;

		if(node->getParentNodes().size()>1) {
			// if unwound, all parents' paths are known
			if(node->needsUnwind()) {
				continue;
			}

			// paths only known if there is an instrumentation in all parents' paths
			for (auto parentNode : node->getParentNodes()) {
				auto n = parentNode;

				if (n->needsInstrumentation() || n->getParentNodes().empty()) {
					continue;
				} else if (n->getParentNodes().size()>1) {
					std::cerr << "Inconsistency in conjunction node: " << node->getFunctionName() << std::endl;
				}
			}
		}
	}
}
