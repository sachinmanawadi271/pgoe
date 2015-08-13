
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

//// INCL STATEMENT COUNT ESTIMATOR PHASE

InclStatementCountEstimatorPhase::InclStatementCountEstimatorPhase(int numberOfStatementsThreshold) :
		EstimatorPhase(std::string("InclStatementCount")+std::to_string(numberOfStatementsThreshold)),
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

		inclStmtCount += node->getNumberOfStatements();

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

// WL CALLPATH DIFFERENTIATION ESTIMATOR PHASE

WLCallpathDifferentiationEstimatorPhase::WLCallpathDifferentiationEstimatorPhase(std::string whiteListName) :
	EstimatorPhase("WLCallpathDifferentiation"),
	whitelistName(whiteListName) {

}

WLCallpathDifferentiationEstimatorPhase::~WLCallpathDifferentiationEstimatorPhase() {}

void WLCallpathDifferentiationEstimatorPhase::modifyGraph(CgNodePtr mainMethod) {

	// TODO: move this parsing somewhere else
	std::ifstream file(whitelistName);
	if (!file) {
		std::cerr << "Error in WLCallpathDifferentiation: Could not open " << whitelistName << std::endl;
		exit(1);
	}
	std::string line;
	while (std::getline(file, line)) {
		if (line.empty()) {
			continue;
		}
		CgNodePtr node = graph->findNode(line);
		if (node == nullptr) {
			continue;
		}
		addNodeAndParentsToWhitelist(node);
	}
	file.close();


	for (auto node : *graph) {
		if (CgHelper::isConjunction(node) && (whitelist.find(node) != whitelist.end()) ) {
			for (auto parentNode : node->getParentNodes()) {
				parentNode->setState(CgNodeState::INSTRUMENT);
			}
		}
	}
}

void WLCallpathDifferentiationEstimatorPhase::addNodeAndParentsToWhitelist(CgNodePtr node) {
	if (whitelist.find(node) == whitelist.end()) {
		whitelist.insert(node);

		for (auto parentNode : node->getParentNodes()) {
			addNodeAndParentsToWhitelist(parentNode);
		}
	}
}

