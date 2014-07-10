
#ifndef CGNODEHELPER_H_
#define CGNODEHELPER_H_

#include <memory>
#include <numeric>	// for std::accumulate
#include "CgNode.h"

// TODO this numbers should be in a config file
namespace CgConfig {
	const unsigned int nanosPerInstrumentedCall = 4;
	const unsigned int nanosPerUnwindSample = 4000;
}

namespace CgHelper {

	bool isConjunction(std::shared_ptr<CgNode> node);
	bool hasUniqueParent(std::shared_ptr<CgNode> node);

	std::shared_ptr<CgNode> getUniqueParent(std::shared_ptr<CgNode> node);

	bool instrumentationCanBeDeleted(std::shared_ptr<CgNode> node);
	bool allParentsPathsInstrumented(std::shared_ptr<CgNode> conjunctionNode);

	unsigned long long getInstrumentationOverheadOfConjunction(std::shared_ptr<CgNode> conjunctionNode);
	unsigned long long getInstumentationOverheadOfPath(std::shared_ptr<CgNode> node);

	bool removeInstrumentationOnPath(std::shared_ptr<CgNode> node);

	bool isConnectedOnSpantree(std::shared_ptr<CgNode> n1, std::shared_ptr<CgNode> n2);
}

#endif
