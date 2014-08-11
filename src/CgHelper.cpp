#include "CgHelper.h"

namespace CgHelper {

	/** returns true for nodes with two or more parents */
	bool isConjunction(CgNodePtr node) {
		return (node->getParentNodes().size() > 1);
	}
	/** returns true for nodes with exactly one parent */
	bool hasUniqueParent(CgNodePtr node) {
		return (node->getParentNodes().size() == 1);
	}

	/** returns the first and unique parent of a node, NO error handling */
	CgNodePtr getUniqueParent(CgNodePtr node) {
		return *(node->getParentNodes().begin());
	}

	/** returns overhead of the call path of a node */
	unsigned long long getInstrumentationOverheadOfPath(CgNodePtr node) {
		auto instrumentedNode = getInstrumentedNodeOnPath(node);

		if (instrumentedNode) {
			if(instrumentedNode->isRootNode()) {
				return 0;	// main() is implictly instrumented
			}
			return instrumentedNode->getNumberOfCalls() * CgConfig::nanosPerInstrumentedCall;
		} else {
			return 0;
		}
	}

	/** returns a pointer to the node that is instrumented up that call path */
	// TODO: check because of new nodeBased Conventions
	CgNodePtr getInstrumentedNodeOnPath(CgNodePtr node) {
		// XXX RN: this method has slowly grown up to a real mess
		if (node->isInstrumented() || node->isRootNode()) {
			return node;
		}

		if (isConjunction(node)) {
			return NULL;
		}
		// single parent
		auto parentNode = getUniqueParent(node);

		// if the parent has multiple children, instrumentation cannot be moved up there
//		if (parentNode->getChildNodes().size() > 1) {
//			return NULL;
//		}

		return getInstrumentedNodeOnPath(parentNode);
	}

	/**
	 * returns true if the instrumentation at a node can be deleted
	 * and all call paths can still be reconstructed.
	 * The node has to be the direct parent of a call conjunction.
	 * Checks the call path reconstruction for all child nodes.
	 */
	// TODO: check because of new nodeBased Conventions
	bool instrumentationCanBeDeleted(CgNodePtr node) {
		for (auto childNode : node->getChildNodes()) {

			if (	   isConjunction(childNode)
					&& !childNode->isUnwound()
					&& !allParentsPathsInstrumented(childNode)) {
				return false;
			}

			for (auto parentOfChildNode : childNode->getParentNodes()) {
				auto instrumentedParent = getInstrumentedNodeOnPath(parentOfChildNode);

				// if the to be removed instrumentation can be reached from another instrumented nodes
				// the call paths can not be reconstructed any longer
				if(		   instrumentedParent
						&& !parentOfChildNode->isSameFunction(node)
						&& reachableFrom(instrumentedParent, node)) {
					return false;
				}
			}
		}
		return true;
	}

	CgNodePtrSet getInstrumentationPath(CgNodePtr start) {
		// TODO does this break for circles? it shouldn't because there can't be uninstrumented circles
		CgNodePtrSet path = {start};

		if (start->isInstrumented()) {
			return path;
		}
		if (start->isRootNode()) {
			return path;
		}
		if (hasUniqueParent(start)) {

			auto parentNode = getUniqueParent(start);
			auto parentPath = getInstrumentationPath(parentNode);
			path.insert(parentPath.begin(), parentPath.end());
			return path;
		}

		if (isConjunction(start)) {
			for (auto parentNode : start->getParentNodes()) {

				auto parentPath = getInstrumentationPath(parentNode);
				path.insert(parentPath.begin(), parentPath.end());
			}

			return path;
		}

		std::cout << "Error: Unknown case in getInstrumentationPath()"
				<< "start : " << *start << std::endl;
		return CgNodePtrSet();
	}

	/**
	 * Checks that the instrumentation paths above a conjunction node do not intersect.
	 * Returns the Number Of Errors ! */
	bool uniqueInstrumentationTest(CgNodePtr conjunctionNode) {

		int numberOfErrors = 0;

		auto parents = conjunctionNode->getParentNodes();
		std::map<CgNodePtr, CgNodePtrSet> paths;

		for (auto parentNode : parents) {
			CgNodePtrSet path = getInstrumentationPath(parentNode);
			paths[parentNode] = path;
		}

		for (auto pair : paths) {
			for (auto otherPair : paths) {

				if (pair==otherPair) {	continue; }

				auto intersection = set_intersect(pair.second, otherPair.second);

				if (!intersection.empty()) {

					std::cout << "ERROR in conjunction: " << *conjunctionNode << std::endl;
					std::cout << "    " << "Paths of " << *(pair.first)
							<< " and " << *(otherPair.first) << " intersect!" << std::endl;

					numberOfErrors++;
				}
			}
		}

		return numberOfErrors;
	}

	/** returns true if the call paths of all parents of a conjunction are instrumented */
	bool allParentsPathsInstrumented(CgNodePtr conjunctionNode) {
		auto parents = conjunctionNode->getParentNodes();

		return std::accumulate(parents.begin(), parents.end(), true,
				[] (bool b, CgNodePtr parent) {
					return b && (getInstrumentationOverheadOfPath(parent)!=0);
				});
	}

	/** returns the overhead caused by a call path */
	// TODO: check because of new nodeBased Conventions
	unsigned long long getInstrumentationOverheadOfConjunction(
			CgNodePtr conjunctionNode) {

		auto parents = conjunctionNode->getParentNodes();

		return std::accumulate(parents.begin(), parents.end(), 0ULL,
				[] (unsigned long long acc, CgNodePtr parent) {
					return acc + getInstrumentationOverheadOfPath(parent);
				});
	}

	/** removes the instrumentation of a call path.
	 * 	returns false if no instrumentation found */
	// TODO: check because of new nodeBased Conventions
	bool removeInstrumentationOnPath(CgNodePtr node) {
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

	bool reachableFrom(CgNodePtr parentNode, CgNodePtr childNode) {

		// XXX RN: once again code duplication
		CgNodePtrSet visitedNodes;
		std::queue<CgNodePtr> workQueue;
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
	bool isConnectedOnSpantree(CgNodePtr n1, CgNodePtr n2) {

		CgNodePtrSet reachableNodes = {n1};
		size_t size = 0;

		while (size != reachableNodes.size()) {

			size = reachableNodes.size();

			// RN:	note that elements are inserted during iteration
			// 		bad things may happen if an unordered container is used here
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

	bool canReachSameConjunction(CgNodePtr n1, CgNodePtr n2) {

		CgNodePtrSet n1Childs = getDescendants(n1);
		CgNodePtrSet n2Childs = getDescendants(n2);

		CgNodePtrSet childIntersect = set_intersect(n1Childs, n2Childs);

		CgNodePtrSet n1Ancestors = getAncestors(n1);
		CgNodePtrSet n2Ancestors = getAncestors(n2);

		CgNodePtrSet ancestorIntersect = set_intersect(n1Ancestors, n2Ancestors);
		return !childIntersect.empty() || !ancestorIntersect.empty();
	}

	CgNodePtrSet getDescendants(CgNodePtr startingNode) {

		CgNodePtrSet childs;
		std::queue<CgNodePtr> workQueue;
		workQueue.push(startingNode);

		while (!workQueue.empty()) {

			auto node = workQueue.front();
			workQueue.pop();

			childs.insert(node);

			for (auto childNode : node->getChildNodes()) {
				if (childNode->isSpantreeParent(node)
						&& childs.find(childNode) == childs.end()) {
					workQueue.push(childNode);
				}
			}
		}

		return childs;
	}

	CgNodePtrSet getAncestors(CgNodePtr startingNode) {

		CgNodePtrSet ancestors;
		std::queue<CgNodePtr> workQueue;
		workQueue.push(startingNode);

		while (!workQueue.empty()) {

			auto node = workQueue.front();
			workQueue.pop();

			ancestors.insert(node);

			for (auto parentNode : node->getParentNodes()) {
				if (node->isSpantreeParent(parentNode)
						&& ancestors.find(parentNode) == ancestors.end()) {
					workQueue.push(parentNode);
				}
			}
		}

		return ancestors;
	}

}
