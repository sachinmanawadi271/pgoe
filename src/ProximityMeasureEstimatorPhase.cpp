#include "ProximityMeasureEstimatorPhase.h"

ProximityMeasureEstimatorPhase::ProximityMeasureEstimatorPhase(
    std::string filename)
    : EstimatorPhase("proximity-estimator"), filename(filename),
      compareAgainst(CubeCallgraphBuilder::build(filename, 1)) {
				std::cerr << "Constructed Proximity Estimator" << std::endl;
			}

void
ProximityMeasureEstimatorPhase::modifyGraph(CgNodePtr mainMethod){
	std::cerr << "Running estimation" << std::endl;
  long numFuncsFull = graph->size();
  long numFuncsOther = compareAgainst.size();
  std::cout << numFuncsFull / numFuncsOther
            << "\% of the originally recorded functions preserved" << std::endl;

  // we get the node ptr from the complete profile
	std::cout << "Children preserved in profile: " << childrenPreservedMetric(mainMethod) << std::endl;
}

void ProximityMeasureEstimatorPhase::prepareList(CgNodePtr mainM){
	worklist.insert(mainM);
	for(const auto n : mainM->getChildNodes()){
		if(worklist.find(n) == worklist.end())
			prepareList(n);
	}
}

double
ProximityMeasureEstimatorPhase::childrenPreservedMetric(CgNodePtr origFunc){

	prepareList(mainMethod);
	std::cout << "Profile has " << worklist.size() << " nodes" << std::endl;

	double val = 0.0;
	// for all functions in original CG, find corresponding node in filtered CG
	// if node is found calculate childrenPreserved(orig, filtered)
	for(auto &origNode : worklist){
		for(auto fNode = compareAgainst.begin(); fNode != compareAgainst.end(); ++fNode){
			
			CgNodePtr funcNode = *fNode;
			
			if(origNode->isSameFunction(funcNode)){
				double cs = childrenPreserved(origNode, funcNode);
				val += cs;
			}
		}
	}
	return val / worklist.size();
}

double
ProximityMeasureEstimatorPhase::childrenPreserved(CgNodePtr orig, CgNodePtr filtered){
	if(orig->getChildNodes().size() == 0){
		if(filtered->getChildNodes().size() > 0){
			std::cerr << "Warning: No child nodes in original profile but filtered profile." << std::endl;
		}
		return 1.0;
	}
	return double(filtered->getChildNodes().size()) / orig->getChildNodes().size();
}

