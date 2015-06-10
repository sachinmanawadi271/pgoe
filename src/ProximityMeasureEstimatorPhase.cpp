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

double
ProximityMeasureEstimatorPhase::childrenPreservedMetric(CgNodePtr origFunc){
	
	if(origFunc.get() == nullptr){
		std::cerr << "ERROR NULLPTR DETECTED" <<std::endl;
	}
	
	if(origFunc->getChildNodes().size() == 0){
		return 100.0;
	}

	double val = 100.0;
	// for all functions in original CG, find corresponding node in filtered CG
	// if node is found calculate childrenPreserved(orig, filtered)
	for(auto origNode : origFunc->getChildNodes()){
		for(auto fNode = compareAgainst.begin(); fNode != compareAgainst.end(); ++fNode){
			auto funcNode = *fNode;
			if((funcNode == nullptr) || (origNode == nullptr)){
				std::cerr << "ERROR NULLPTR DETECTED" << std::endl;
				continue;
			}
			if(origNode->isSameFunction(funcNode)){
				double cs = childrenPreserved(origNode, funcNode);
//				std::cout << "preserved children in " << origNode->getFunctionName() << ":\n\t" << cs << "\n";
				val *= cs;
			}
		}
		if(origNode == nullptr){
			std::cerr << "ERROR NULLPTR DETECTED" <<std::endl;
			continue;
		}
		val *= childrenPreservedMetric(origNode);
	}
	return val;
}

double
ProximityMeasureEstimatorPhase::childrenPreserved(CgNodePtr orig, CgNodePtr filtered){
	if(orig->getChildNodes().size() == 0){
		if(filtered->getChildNodes().size() > 0){
			std::cerr << "Warning: No child nodes in original profile but filtered profile." << std::endl;
		}
		return 1;
	}
	return filtered->getChildNodes().size() / orig->getChildNodes().size();
}

