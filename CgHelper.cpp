#include "CgHelper.h"

namespace CgHelper {

	/** returns true for nodes with two or more parents */
	bool isConjunction(std::shared_ptr<CgNode> node) {
		return (node->getParentNodes().size() > 1);
	}

	/** returns true if either instrumented
	 *  or on unique path that is instrumented above */
	bool isOnInstrumentedPath(std::shared_ptr<CgNode> node) {

		if (node->isInstrumented()) {
			return true;
		}
		if (isConjunction(node) || node->isRootNode()) {
			return false;
		}
		// single parent
		auto parentNode = *(node->getParentNodes().begin());
		return isOnInstrumentedPath(parentNode);
	}

}
