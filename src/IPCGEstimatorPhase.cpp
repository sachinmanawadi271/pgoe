
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

	parentNode->setState(CgNodeState::INSTRUMENT_WITNESS);

	for (auto childNode : parentNode->getChildNodes()) {
		instrumentLevel(childNode, levelsLeft-1);
	}
}

//// STATEMENT COUNT ESTIMATOR PHASE

StatementCountEstimatorPhase::StatementCountEstimatorPhase(int numberOfStatementsThreshold, bool inclusiveMetric) :
		EstimatorPhase((inclusiveMetric ? std::string("Incl") : std::string("Excl"))
				+ std::string("StatementCount")
				+ std::to_string(numberOfStatementsThreshold)),
		numberOfStatementsThreshold(numberOfStatementsThreshold),
		inclusiveMetric(inclusiveMetric) {
}

StatementCountEstimatorPhase::~StatementCountEstimatorPhase() {}

void StatementCountEstimatorPhase::modifyGraph(CgNodePtr mainMethod) {
	for (auto node : *graph) {
		estimateStatementCount(node);
	}
}

void StatementCountEstimatorPhase::estimateStatementCount(CgNodePtr startNode) {

	int inclStmtCount = 0;
	if (inclusiveMetric) {

		// INCLUSIVE
		std::queue<CgNodePtr> workQueue;
		workQueue.push(startNode);
		std::set<CgNodePtr> visitedNodes;

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
		inclStmtCounts[startNode] = inclStmtCount;

	} else {
		// EXCLUSIVE
		inclStmtCount = startNode->getNumberOfStatements();
	}

	if (inclStmtCount > numberOfStatementsThreshold) {
		startNode->setState(CgNodeState::INSTRUMENT_WITNESS);
	}

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
				parentNode->setState(CgNodeState::INSTRUMENT_WITNESS);
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

