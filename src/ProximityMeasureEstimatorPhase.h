#ifndef CUBECALLGRAPHTOOL_PROXIMITYMEASUREESTIMATORPHASE_H
#define CUBECALLGRAPHTOOL_PROXIMITYMEASUREESTIMATORPHASE_H

#include "Callgraph.h"
#include "CubeReader.h"
#include "EstimatorPhase.h"


class ProximityMeasureEstimatorPhase : public EstimatorPhase {

public:
  ProximityMeasureEstimatorPhase(std::string filename);
  virtual void modifyGraph(CgNodePtr mainMethod) override;
	double childrenPreservedMetric(CgNodePtr origFunc);

private:
	double childrenPreserved(CgNodePtr orig, CgNodePtr filtered);
	void prepareList(CgNodePtr mainM);

	std::string filename;
	CallgraphManager compareAgainst;
	std::set<CgNodePtr> worklist;
};

#endif
