
#include "IPCGEstimatorPhase.h"

FirstNLevelsEstimatorPhase::FirstNLevelsEstimatorPhase(int levels) :
		EstimatorPhase(std::string("FirstNLevels")+std::to_string(levels)),
		levels(levels) {
}

FirstNLevelsEstimatorPhase::~FirstNLevelsEstimatorPhase() {
}

void FirstNLevelsEstimatorPhase::modifyGraph(CgNodePtr mainMethod) {
	instrumentLevel(mainMethod, levels);
}

void FirstNLevelsEstimatorPhase::instrumentLevel(CgNodePtr parentNode, int levelsLeft) {

	if (levelsLeft == 0) {
		return;
	}

	parentNode->setState(CgNodeState::INSTRUMENT);

	for (auto childNode : parentNode->getChildNodes()) {
		instrumentLevel(childNode, levelsLeft-1);
	}
}


void FirstNLevelsEstimatorPhase::printAdditionalReport() {
}


//void FirstNLevelsEstimatorPhase::printReport() {
//	// only print the additional report
//	printAdditionalReport();
//}


//// INCL STATEMENT COUNT ESTIMATOR PHASE

InclStatementCountEstimatorPhase::InclStatementCountEstimatorPhase(int numberOfStatementsThreshold) :
		EstimatorPhase("InclStatementCount"),
		numberOfStatementsThreshold(numberOfStatementsThreshold) {
}

InclStatementCountEstimatorPhase::~InclStatementCountEstimatorPhase() {}

void InclStatementCountEstimatorPhase::modifyGraph(CgNodePtr mainMethod) {
	for (auto node : *graph) {
		estimateInclStatementCount(node);
	}
}

void InclStatementCountEstimatorPhase::estimateInclStatementCount(CgNodePtr startNode) {
	std::queue<CgNodePtr> workQueue;
	workQueue.push(startNode);
	std::set<CgNodePtr> visitedNodes;

	int inclStmtCount = 0;

	while (!workQueue.empty()) {
		auto node = workQueue.front();
		workQueue.pop();

		visitedNodes.insert(node);

		inclStmtCount += node->getLinesOfCode();

		for (auto childNode : node->getChildNodes()) {
			if (visitedNodes.find(childNode) == visitedNodes.end()) {
				workQueue.push(childNode);
			}
		}
	}

	if (inclStmtCount > numberOfStatementsThreshold) {
		startNode->setState(CgNodeState::INSTRUMENT);
	}

	inclStmtCounts[startNode] = inclStmtCount;
}

void InclStatementCountEstimatorPhase::printAdditionalReport() {
}
