
#ifndef SANITYCHECKESTIMATORPHASE_H_
#define SANITYCHECKESTIMATORPHASE_H_

#include <queue>

#include "EstimatorPhase.h"
#include "CgHelper.h"

/**
 * Does not modify the graph.
 * Only does a full sanity checks for instrumentation & unwind.
 */
class SanityCheckEstimatorPhase : public EstimatorPhase {
public:
	SanityCheckEstimatorPhase(CgNodePtrSet* graph);
	~SanityCheckEstimatorPhase();
	/** does NOT modify the graph */
	void modifyGraph(CgNodePtr mainMethod);

	void printReport();

private:
	int numberOfErrors;

	void printAdditionalReport();
};


#endif
