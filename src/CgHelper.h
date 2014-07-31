
#ifndef CGNODEHELPER_H_
#define CGNODEHELPER_H_

#include <memory>
#include <queue>
#include <numeric>	// for std::accumulate
#include "CgNode.h"

// TODO this numbers should be in a config file
namespace CgConfig {
	const unsigned int nanosPerInstrumentedCall = 4;

	const unsigned int nanosPerUnwindSample 	= 4000;
	const unsigned int nanosPerUnwindStep 		= 200;
}

namespace CgHelper {

	bool isConjunction(CgNodePtr node);
	bool hasUniqueParent(CgNodePtr node);

	CgNodePtr getUniqueParent(CgNodePtr node);

	bool instrumentationCanBeDeleted(CgNodePtr node);
	bool allParentsPathsInstrumented(CgNodePtr conjunctionNode);

	unsigned long long getInstrumentationOverheadOfConjunction(CgNodePtr conjunctionNode);
	unsigned long long getInstrumentationOverheadOfPath(CgNodePtr node);
	CgNodePtr getInstrumentedNodeOnPath(CgNodePtr node);

	bool reachableFrom(CgNodePtr parentNode, CgNodePtr childNode);

	bool removeInstrumentationOnPath(CgNodePtr node);

	bool isConnectedOnSpantree(CgNodePtr n1, CgNodePtr n2);
}

#endif
