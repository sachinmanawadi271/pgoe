
#include "OptimalNodeBasedEstimatorPhase.h"

OptimalNodeBasedEstimatorPhase::OptimalNodeBasedEstimatorPhase() :
		EstimatorPhase("OptimalNodeBased"),
		optimalCosts(INT64_MAX){
}

OptimalNodeBasedEstimatorPhase::~OptimalNodeBasedEstimatorPhase() {
}

void OptimalNodeBasedEstimatorPhase::modifyGraph(CgNodePtr mainMethod) {

	findStartingState(mainMethod);
	///XXX
	std::cout << " # Found starting state" << std::endl;

	while (!stateStack.empty()) {

		///XXX
		std::cout << "  #loop" << std::endl;

		// XXX RN i'm not sure this will work as intended if the stack is modified
		for (auto node : stateStack.top().currentInstrumentation) {
			auto parentNodes = node->getParentNodes();

			auto newState(stateStack.top());

			if (newState.validAfterExchange(node, parentNodes)) {
				// TODO: calculate costs

				stateStack.push(newState);
			}

		}

		stateStack.pop();	// XXX is this correct?
	}

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
