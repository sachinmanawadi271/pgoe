
#ifndef MINIMALSPANTREEESTIMATORPHASE_H_
#define MINIMALSPANTREEESTIMATORPHASE_H_

#include "EstimatorPhase.h"

struct SpantreeEdge {
	int calls;
	std::shared_ptr<CgNode> child;
	std::shared_ptr<CgNode> parent;
};

struct MoreCalls {
	bool operator() (const SpantreeEdge& lhs, const SpantreeEdge& rhs) {
		return lhs.calls >= rhs.calls;
	}
};

class MinimalSpantreeEstimatorPhase : public EstimatorPhase {
public:
	MinimalSpantreeEstimatorPhase(std::map<std::string, std::shared_ptr<CgNode> >* graph);
	~MinimalSpantreeEstimatorPhase();

	void modifyGraph(std::shared_ptr<CgNode> mainMethod);

	void printReport();
protected:
	void printAdditionalReport();
//private:
//	bool graphHasCircle(std::shared_ptr<CgNode> mainMethod);
};

#endif
