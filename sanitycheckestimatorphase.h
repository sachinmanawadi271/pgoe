
#ifndef SANITYCHECKESTIMATORPHASE_H_
#define SANITYCHECKESTIMATORPHASE_H_

#include <iostream>

#include "estimatorphase.h"


/**
 * Does not modify the graph.
 * Only does a full sanity checks for instrumentation & unwind.
 */
class SanityCheckEstimatorPhase : public EstimatorPhase {
public:
	SanityCheckEstimatorPhase(std::map<std::string, std::shared_ptr<CgNode> >* graph);
	~SanityCheckEstimatorPhase();
	/** does NOT modify the graph */
	void modifyGraph(std::shared_ptr<CgNode> mainMethod);
};


#endif
