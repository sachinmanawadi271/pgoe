
#include "OptimalNodeBasedEstimatorPhase.h"

#define DEBUG 1

OptimalNodeBasedEstimatorPhase::OptimalNodeBasedEstimatorPhase() :
		EstimatorPhase("OptimalNodeBased"),
		optimalCosts(INT64_MAX){
}

OptimalNodeBasedEstimatorPhase::~OptimalNodeBasedEstimatorPhase() {
}

void OptimalNodeBasedEstimatorPhase::step() {

	if (stateStack.empty()) {
		return;
	}

#if DEBUG
	std::cout << "+ push " << stateStack.top() << std::endl;
#endif

	for (auto node : stateStack.top().nodeSet) {

		if (node->isRootNode()) {
			continue;
		}

		auto parentNodes = node->getParentNodes();
		auto newState(stateStack.top());

#if DEBUG
		std::cout << "   " << "+ try switching \"" << node->getFunctionName()
				<< "\" for:  [";
		for (auto n : parentNodes) {
			std::cout << "\"" << n->getFunctionName() << "\", ";
		}
		std::cout << "] ";
#endif

		if (!parentNodes.empty() && newState.validAfterExchange(node, parentNodes)) {
			unsigned long long costs = newState.getCosts();

			if (costs < optimalCosts) {
				optimalCosts = costs;
				optimalInstrumentation = newState.nodeSet;
			}
#if DEBUG
			std::cout << "--> success" << std::endl;
#endif

			stateStack.push(newState);
			step();

		} else {
#if DEBUG
			std::cout << "--> fail" << std::endl;
#endif
		}
	}

	stateStack.pop();
#if DEBUG
	if (!stateStack.empty()) {
			std::cout << "+ pop  " << stateStack.top() << std::endl;
	}
#endif

}

void OptimalNodeBasedEstimatorPhase::modifyGraph(CgNodePtr mainMethod) {

	findStartingState(mainMethod);

	step();

	for (auto node : optimalInstrumentation) {
		node->setState(CgNodeState::INSTRUMENT);
	}
	mainMethod->setState(CgNodeState::NONE);	// main() is implicitly instrumented

}

void OptimalNodeBasedEstimatorPhase::printAdditionalReport() {
	// TODO additional report
}

/** fill the initial stack with all parents of conjunctions and all initial constraints */
void OptimalNodeBasedEstimatorPhase::findStartingState(CgNodePtr mainMethod) {

	CgNodePtrSet startingParents = {mainMethod};	// main() is implicitly instrumented
	ConstraintContainer startingConstraints;

	for (auto node : (*graph)) {
		if (CgHelper::isConjunction(node)) {

			CgNodePtrSet currentParents;

			for (auto parentNode : node->getParentNodes()) {
				currentParents.insert(parentNode);
			}

			startingParents.insert(currentParents.begin(), currentParents.end());
			startingConstraints.push_back(OptimalNodeBasedConstraint(currentParents));
		}
	}

	optimalInstrumentation = startingParents;
	stateStack.push(OptimalNodeBasedState(startingParents, startingConstraints));
}
