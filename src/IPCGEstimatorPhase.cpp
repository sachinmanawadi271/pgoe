
#include "IPCGEstimatorPhase.h"

FirstNLevelsEstimatorPhase::FirstNLevelsEstimatorPhase(int levels) :
		EstimatorPhase(std::string("FirstNLevels")+std::to_string(levels)),
		levels(levels) {
}

FirstNLevelsEstimatorPhase::~FirstNLevelsEstimatorPhase() {
}

void FirstNLevelsEstimatorPhase::modifyGraph(CgNodePtr mainMethod) {
	instrumentLevel(mainMethod, levels);
}

void FirstNLevelsEstimatorPhase::instrumentLevel(CgNodePtr parentNode, int levelsLeft) {

	if (levelsLeft == 0) {
		return;
	}

	parentNode->setState(CgNodeState::INSTRUMENT);

	for (auto childNode : parentNode->getChildNodes()) {
		instrumentLevel(childNode, levelsLeft-1);
	}
}


void FirstNLevelsEstimatorPhase::printAdditionalReport() {
}


//void FirstNLevelsEstimatorPhase::printReport() {
//	// only print the additional report
//	printAdditionalReport();
//}
