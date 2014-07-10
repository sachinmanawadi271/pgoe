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

	/** returns overhead of the call path of a node */
	unsigned long long getInstumentationOverheadOfPath(std::shared_ptr<CgNode> node) {


		if (node->isInstrumented()) {

			if(node->isRootNode()) {	// main method has no callers
				return CgConfig::nanosPerInstrumentedCall;
			}
			return node->getNumberOfCalls() * CgConfig::nanosPerInstrumentedCall;
		}
		if (isConjunction(node) || node->isRootNode()) {
			return 0;
		}
		// single parent
		auto parentNode = getUniqueParent(node);
		return getInstumentationOverheadOfPath(parentNode);
	}

	/** returns the overhead caused by a call path */
	unsigned long long getInstrumentationOverheadOfConjunction(
			std::shared_ptr<CgNode> conjunctionNode) {

		auto parents = conjunctionNode->getParentNodes();

		return std::accumulate(parents.begin(), parents.end(), 0,
				[] (int i, std::shared_ptr<CgNode> parent) {
					return i + getInstumentationOverheadOfPath(parent);
				});
	}

	/** removes the instrumentation of a call path.
	 * 	returns false if no instrumentation found */
	bool removeInstrumentationOnPath(std::shared_ptr<CgNode> node) {
		if (node->isInstrumented()) {
			node->setState(CgNodeState::NONE);
			return true;
		}
		if (isConjunction(node) || node->isRootNode()) {
			return false;
		}
		return removeInstrumentationOnPath(getUniqueParent(node));
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
