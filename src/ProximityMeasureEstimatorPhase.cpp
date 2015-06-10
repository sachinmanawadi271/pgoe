#include "ProximityMeasureEstimatorPhase.h"

ProximityMeasureEstimatorPhase::ProximityMeasureEstimatorPhase(
    std::string filename)
    : EstimatorPhase("proximity-estimator"), filename(filename),
      compareAgainst(CubeCallgraphBuilder::build(filename, 1)) {
  std::cerr << "Constructed Proximity Estimator" << std::endl;
}

void ProximityMeasureEstimatorPhase::modifyGraph(CgNodePtr mainMethod) {
  std::cerr << "Running estimation" << std::endl;
  long numFuncsFull = graph->size();
  long numFuncsOther = compareAgainst.size();
  std::cout << numFuncsFull / numFuncsOther
            << "\% of the originally recorded functions preserved" << std::endl;

  // we get the node ptr from the complete profile
  std::cout << "Children preserved in profile: "
            << childrenPreservedMetric(mainMethod) << std::endl;
//  std::cout << "Runtime of main compared to its children: "
//            << portionOfRuntime(mainMethod) << std::endl;
  std::cout << "Runtime of functions\n";
  
	workQ.clear();
  for (auto iter = compareAgainst.begin(); iter != compareAgainst.end();
       ++iter) {
		if(workQ.find(*iter) == workQ.end())
    workQ.insert(*iter);
  }

  for (const auto &n : workQ) {
    
		CgNodePtrSet::iterator start =
        std::find_if(graph->begin(), graph->end(), [n](const CgNodePtr &ptr) {
          if (ptr->isSameFunction(n))
            return true;

          return false;
        });

		if(start == graph->end()){
			continue;
		}

    portionOfRuntime(*start);
  }
}

void ProximityMeasureEstimatorPhase::printReport() {
  std::cout << "==== ProximityMeasure Reporter ====\n";
}

double
ProximityMeasureEstimatorPhase::childrenPreservedMetric(CgNodePtr origFunc) {

  prepareList(origFunc);

  double val = 0.0;
  // for all functions in original CG, find corresponding node in filtered CG
  // if node is found calculate childrenPreserved(orig, filtered)
  for (auto &origNode : worklist) {
    CgNodePtr funcNode = getCorrespondingComparisonNode(origNode);

    if (funcNode == nullptr)
      continue;

    double cs = childrenPreserved(origNode, funcNode);
    val += cs;
  }
  return val / worklist.size();
}

// returns the CgNode with the same function as node
CgNodePtr ProximityMeasureEstimatorPhase::getCorrespondingComparisonNode(
    const CgNodePtr node) {
  for (auto fNode = compareAgainst.begin(); fNode != compareAgainst.end();
       ++fNode) {

    CgNodePtr funcNode = *fNode;

    if (node->isSameFunction(funcNode)) {
      return funcNode;
    }
  }
  return nullptr;
}

double ProximityMeasureEstimatorPhase::portionOfRuntime(CgNodePtr node) {

  std::pair<double, double> runtime = getInclusiveAndChildrenRuntime(node);

  CgNodePtr compNode = getCorrespondingComparisonNode(node);
  if (compNode == nullptr) {
    return 0.0;
  }

  std::pair<double, double> runtimeInCompareProfile =
      getInclusiveAndChildrenRuntime(compNode);

  std::cout.precision(12);
  std::cout << "====\nRuntime: " << runtime.first
            << "\nCompared runtime: " << runtimeInCompareProfile.first
            << "\nRuntime of children: " << runtime.second
            << "\nRuntime of children: " << runtimeInCompareProfile.second
            << "\n====" << std::endl;
  std::cout << "==== Ratio between runtimes ====\nOriginal profile: "
            << runtime.second / runtime.first << "\nCompared profile: "
            << runtimeInCompareProfile.second / runtimeInCompareProfile.first
            << "\n" << std::endl;

  return runtime.second / runtime.first;
}

std::pair<double, double>
ProximityMeasureEstimatorPhase::getInclusiveAndChildrenRuntime(CgNodePtr node) {
  worklist.clear();
  prepareList(node);

  double runtimeInSeconds = node->getRuntimeInSeconds();
  // the init value is -runtimeInSeconds, because worklist contains "node"
  // which we don't want to include in the summation
  double runtimeSumOfChildrenInSeconds =
      std::accumulate(worklist.begin(), worklist.end(), -runtimeInSeconds,
                      [](const double &a, const CgNodePtr &b) {
                        return a + b->getRuntimeInSeconds();
                      });

  runtimeInSeconds += runtimeSumOfChildrenInSeconds;
  return std::make_pair(runtimeInSeconds, runtimeSumOfChildrenInSeconds);
}

double ProximityMeasureEstimatorPhase::childrenPreserved(CgNodePtr orig,
                                                         CgNodePtr filtered) {
  if (orig->getChildNodes().size() == 0) {
    if (filtered->getChildNodes().size() > 0) {
      std::cerr << "[Warning]: Should not happen!\n\t>>No child nodes in "
                   "original profile but filtered profile." << std::endl;
    }
    return 1.0;
  }
  return double(filtered->getChildNodes().size()) /
         orig->getChildNodes().size();
}

void ProximityMeasureEstimatorPhase::prepareList(CgNodePtr mainM) {
  worklist.insert(mainM);
  for (const auto n : mainM->getChildNodes()) {
    if (worklist.find(n) == worklist.end())
      prepareList(n);
  }
}

