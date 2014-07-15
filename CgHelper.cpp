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
		// XXX RN: this method has slowly grown up to a real mess
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

		// if the parent has multiple children, instrumentation cannot be moved up there
		if (parentNode->getChildNodes().size() > 1) {
			return 0;
		}

		return getInstumentationOverheadOfPath(parentNode);
	}

	std::shared_ptr<CgNode> getInstrumentedNodeOnPath(std::shared_ptr<CgNode> node) {
		if (node->isInstrumented()) {
			return node;
		}

		if (isConjunction(node) || node->isRootNode()) {
			return NULL;
		}
		// single parent
		auto parentNode = getUniqueParent(node);

		// if the parent has multiple children, instrumentation cannot be moved up there
		if (parentNode->getChildNodes().size() > 1) {
			return NULL;
		}

		return getInstrumentedNodeOnPath(parentNode);
	}

	bool instrumentationCanBeDeleted(std::shared_ptr<CgNode> node) {
		for (auto childNode : node->getChildNodes()) {
			if (	   isConjunction(childNode)
					&& !childNode->isUnwound()
					&& !allParentsPathsInstrumented(childNode)) {
				return false;
			}
		}
		return true;
	}

	/** returns true if the call paths of all parents of a conjunction are instrumented */
	bool allParentsPathsInstrumented(std::shared_ptr<CgNode> conjunctionNode) {
		auto parents = conjunctionNode->getParentNodes();

		return std::accumulate(parents.begin(), parents.end(), true,
				[] (bool b, std::shared_ptr<CgNode> parent) {
					return b && (getInstumentationOverheadOfPath(parent)!=0);
				});
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

		// there can not be instrumentation up here if the parent has multiple children
		auto uniqueParent = getUniqueParent(node);
		if (uniqueParent->getChildNodes().size()>1) {
			return false;
		}

		return removeInstrumentationOnPath(getUniqueParent(node));
	}

	bool reachableFrom(std::shared_ptr<CgNode> parentNode, std::shared_ptr<CgNode> childNode) {

		std::set<std::shared_ptr<CgNode> > visitedNodes;
		std::queue<std::shared_ptr<CgNode> > workQueue;
		workQueue.push(parentNode);

		while (!workQueue.empty()) {

			auto node = workQueue.front();
			workQueue.pop();

			if (node->isSameFunction(childNode)) {
				return true;
			}

			visitedNodes.insert(node);

			for (auto childNode : node->getChildNodes()) {
				if (visitedNodes.find(childNode) == visitedNodes.end()) {
					workQueue.push(childNode);
				}
			}
		}

		return false;
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
