#ifndef IPCGESTIMATORPHASE_H_
#define IPCGESTIMATORPHASE_H_

#include "EstimatorPhase.h"

#include <map>
#include <queue>
#include <set>


/** RN: instrument the first n levels starting from main */
class FirstNLevelsEstimatorPhase : public EstimatorPhase {
public:
	FirstNLevelsEstimatorPhase(int levels);
	~FirstNLevelsEstimatorPhase();

	void modifyGraph(CgNodePtr mainMethod);

private:
	void instrumentLevel(CgNodePtr parentNode, int levelsLeft);

	const int levels;
};

/**
 * RN: An optimistic inclusive statement count heuristic.
 * Sums up statement count for all reachable nodes from a startNode.
 * Cycles are counted only once.
 * Edge counts are NOT taken into account.
 */
class StatementCountEstimatorPhase : public EstimatorPhase {
public:
	StatementCountEstimatorPhase(int numberOfStatementsThreshold, bool inclusiveMetric = true);
	~StatementCountEstimatorPhase();

	void modifyGraph(CgNodePtr mainMethod);
	void estimateStatementCount(CgNodePtr startNode);

private:
	int numberOfStatementsThreshold;
	bool inclusiveMetric;
	std::map<CgNodePtr, int> inclStmtCounts;
};

class RuntimeEstimatorPhase : public EstimatorPhase{
public:
	RuntimeEstimatorPhase( double runTimeThreshold, bool inclusiveMetric = true);
	~RuntimeEstimatorPhase();

	void modifyGraph(CgNodePtr mainMethod);
	void estimateRuntime(CgNodePtr startNode);

private:
    double runTimeThreshold;
	bool inclusiveMetric;
    std::map<CgNodePtr,double> inclRunTime;
};

/**
 * RN: Gets a file with a whitelist of interesting nodes.
 * Instruments all paths to these nodes with naive callpathDifferentiation.
 */
class WLCallpathDifferentiationEstimatorPhase : public EstimatorPhase {
public:
	WLCallpathDifferentiationEstimatorPhase(std::string whiteListName="whitelist.txt");
	~WLCallpathDifferentiationEstimatorPhase();

	void modifyGraph(CgNodePtr mainMethod);
private:
	CgNodePtrSet whitelist;	// all whitelisted nodes INCL. their paths to main
	std::string whitelistName;

	void addNodeAndParentsToWhitelist(CgNodePtr node);
};

#endif
