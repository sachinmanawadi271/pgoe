
#ifndef EDGEBASEDOPTIMUMESTIMATORPHASE_H_
#define IPCGESTIMATORPHASE_H_

#include "EstimatorPhase.h"
#include "CgHelper.h"


/**
 * RN:
 */
class FirstNLevelsEstimatorPhase : public EstimatorPhase {
public:
	FirstNLevelsEstimatorPhase(int levels);
	~FirstNLevelsEstimatorPhase();

	void modifyGraph(CgNodePtr mainMethod);

//	void printReport();
protected:
	void printAdditionalReport();

private:
	void instrumentLevel(CgNodePtr parentNode, int levelsLeft);

	const int levels;
};

#endif
