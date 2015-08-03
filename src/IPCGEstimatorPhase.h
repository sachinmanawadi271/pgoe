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

//	void printReport();
protected:
	void printAdditionalReport();

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
class InclStatementCountEstimatorPhase : public EstimatorPhase {
public:
	InclStatementCountEstimatorPhase(int numberOfStatementsThreshold);
	~InclStatementCountEstimatorPhase();

	void modifyGraph(CgNodePtr mainMethod);
	void estimateInclStatementCount(CgNodePtr startNode);

protected:
	void printAdditionalReport();
private:
	int numberOfStatementsThreshold;
	std::map<CgNodePtr, int> inclStmtCounts;
};

#endif
