
#ifndef MINIMALSPANTREEESTIMATORPHASE_H_
#define MINIMALSPANTREEESTIMATORPHASE_H_

#include "EstimatorPhase.h"
#include "CgHelper.h"

struct SpantreeEdge {
	unsigned long long calls;
	std::shared_ptr<CgNode> child;
	std::shared_ptr<CgNode> parent;
};

struct MoreCalls {
	bool operator() (const SpantreeEdge& lhs, const SpantreeEdge& rhs) {
		return lhs.calls < rhs.calls;
	}
};

/**
 * RN: this phase can run independent of all others except for RemoveUnrelatedNodes
 * The results turned out to be less optimal than we expected
 */
class MinimalSpantreeEstimatorPhase : public EstimatorPhase {
public:
	MinimalSpantreeEstimatorPhase(std::map<std::string, std::shared_ptr<CgNode> >* graph);
	~MinimalSpantreeEstimatorPhase();

	void modifyGraph(std::shared_ptr<CgNode> mainMethod);

	void printReport();
protected:
	void printAdditionalReport();
private:
	int numberOfSkippedEdges;
};

#endif
