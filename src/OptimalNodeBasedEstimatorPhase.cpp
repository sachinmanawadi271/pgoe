
#include "OptimalNodeBasedEstimatorPhase.h"

#define DEBUG 0
#define USE_OPTIMIZED_ORDER 1	// XXX in the long term both cases should lead to the same results

OptimalNodeBasedEstimatorPhase::OptimalNodeBasedEstimatorPhase() :
		EstimatorPhase("OptimalNodeBased"),
		optimalCosts(INT64_MAX),
		numberOfStepsTaken(0),
		numberOfStepsAvoided(0) {
}

OptimalNodeBasedEstimatorPhase::~OptimalNodeBasedEstimatorPhase() {
}

void OptimalNodeBasedEstimatorPhase::step(OptimalNodeBasedState& startState) {

	// skip already visited combinations
	std::size_t hash = std::hash<OptimalNodeBasedState>()(startState);
	if (visitedCombinations.find(hash) != visitedCombinations.end()) {
		numberOfStepsAvoided++;
		return;	// this combination has already been visited
	}
	visitedCombinations.insert(hash);

	numberOfStepsTaken++;
	///XXX
	if(numberOfStepsTaken % 1000 == 0) {
		std::cout << numberOfStepsTaken << " steps taken ("
				<< numberOfStepsAvoided << " avoided) " << std::endl;
	}

#if USE_OPTIMIZED_ORDER
	// order the nodes by their weight, then start to replace the most expensive
	std::priority_queue<CgNodePtr, std::vector<CgNodePtr>, CalledMoreOften> pq;
	for (auto node : startState.nodeSet) {
		pq.push(node);
	}
	while (!pq.empty()) {
		auto node = pq.top();
		pq.pop();

#else
	for (auto node : startState.nodeSet) {
#endif

		if (node->isRootNode()) {
			continue;
		}

		auto parentNodes = node->getParentNodes();
		auto newState(startState);

#if DEBUG
		std::cout << "   " << "+ try switching " << *node	<< " for:  [";
		for (auto n : parentNodes) {
			std::cout << *n << ", ";
		}
		std::cout << "] ";
#endif

		if (!parentNodes.empty() && newState.validAfterExchange(node, parentNodes)) {
			unsigned long long costs = newState.getCosts();

			if (costs < optimalCosts) {

				///XXX
				std::cout << "minimum: "
						<< optimalCosts*CgConfig::nanosPerInstrumentedCall/1e9 << " s" << std::endl;

				optimalCosts = costs;
				optimalInstrumentation = newState.nodeSet;
			}
#if DEBUG
			std::cout << "--> success" << std::endl;
#endif
			step(newState);

		} else {
#if DEBUG
			std::cout << "--> fail" << std::endl;
#endif
		}
	}
}

void OptimalNodeBasedEstimatorPhase::modifyGraph(CgNodePtr mainMethod) {

	findStartingState(mainMethod);

	step(stateStack.top());

	for (auto node : optimalInstrumentation) {
		node->setState(CgNodeState::INSTRUMENT);
	}
	mainMethod->setState(CgNodeState::NONE);	// main() is implicitly instrumented

}

void OptimalNodeBasedEstimatorPhase::printAdditionalReport() {
	std::cout << "\t" << "computation steps taken: " << numberOfStepsTaken
			<< " (avoided " << numberOfStepsAvoided << ")" << std::endl;
}

/** fill the initial stack with all parents of conjunctions and all initial constraints */
void OptimalNodeBasedEstimatorPhase::findStartingState(CgNodePtr mainMethod) {

	CgNodePtrSet startingParents = {mainMethod};	// main() is implicitly instrumented
	ConstraintContainer startingConstraints;

	for (auto node : (*graph)) {
		if (CgHelper::isConjunction(node)) {

			CgNodePtrSet parentNodes = node->getParentNodes();

			startingParents.insert(parentNodes.begin(), parentNodes.end());
			startingConstraints.push_back(OptimalNodeBasedConstraint(parentNodes, node));
		}
	}

	auto startingState = OptimalNodeBasedState(startingParents, startingConstraints);

	optimalInstrumentation = startingParents;
	optimalCosts = startingState.getCosts();

	stateStack.push(startingState);
}


