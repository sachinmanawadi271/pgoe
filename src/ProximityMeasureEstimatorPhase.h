#ifndef CUBECALLGRAPHTOOL_PROXIMITYMEASUREESTIMATORPHASE_H
#define CUBECALLGRAPHTOOL_PROXIMITYMEASUREESTIMATORPHASE_H

#include "Callgraph.h"
#include "CubeReader.h"
#include "EstimatorPhase.h"

class ProximityMeasureEstimatorPhase : public EstimatorPhase {

public:
  ProximityMeasureEstimatorPhase(std::string filename);
  virtual void modifyGraph(CgNodePtr mainMethod) override;

  /**
   * Calculates for each node n in profile starting from origFunc
   * val = sum_{F}(childrenPreserved(F, G))/#Func
   * with F being all functions in the subtree starting at origFunc
   * and G being all corresponding functions in the filtered profile
   *
   * \returns a percentage value how many calls were preserved
   */
  double childrenPreservedMetric(CgNodePtr origFunc);
  double portionOfRuntime(CgNodePtr node);

	void printReport() override;

private:
  double childrenPreserved(CgNodePtr orig, CgNodePtr filtered);
  void prepareList(CgNodePtr mainM);
  std::pair<double, double> getExclusiveAndChildrenRuntime(CgNodePtr node);
	CgNodePtr getCorrespondingComparisonNode(const CgNodePtr node);


  std::string filename;
  CallgraphManager compareAgainst;
  std::set<CgNodePtr> worklist;
};

#endif
