#include "CgHelper.h"

namespace CgHelper {

	/** returns true for nodes with two or more parents */
	bool isConjunction(std::shared_ptr<CgNode> node) {
		return (node->getParentNodes().size() > 1);
	}
	/** returns true for nodes with exactly one parent */
	bool hasUniqueParent(std::shared_ptr<CgNode> node) {
		return (node->getParentNodes().size() == 1);
	}

	/** returns the first and unique parent of a node, NO error handling */
	std::shared_ptr<CgNode> getUniqueParent(std::shared_ptr<CgNode> node) {
		return *(node->getParentNodes().begin());
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

	/** true if the two nodes are connected via spanning tree edges */
	// XXX RN: this method is ugly and has horrible complexity
	bool isConnectedOnSpantree(std::shared_ptr<CgNode> n1, std::shared_ptr<CgNode> n2) {

		std::set<std::shared_ptr<CgNode> > reachableNodes;
		reachableNodes.insert(n1);

		size_t size = 0;
		while (size < reachableNodes.size()) {

			size = reachableNodes.size();

			for (auto node : reachableNodes) {

				for (auto parentNode : node->getParentNodes()) {
					if(node->isSpantreeParent(parentNode)) {
						reachableNodes.insert(parentNode);
					}
				}
				for (auto childNode : node->getChildNodes()) {
					if (childNode->isSpantreeParent(node)) {
						reachableNodes.insert(childNode);
					}
				}
			}
		}

		return reachableNodes.find(n2) != reachableNodes.end();
	}

}
