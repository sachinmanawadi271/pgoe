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

	/** returns true if the call paths of all parents of a conjunction are instrumented */
	bool allParentsPathsInstrumented(CgNodePtr conjunctionNode) {
		auto parents = conjunctionNode->getParentNodes();

		return std::accumulate(parents.begin(), parents.end(), true,
				[] (bool b, CgNodePtr parent) {
					return b && (getInstrumentationOverheadOfPath(parent)!=0);
				});
	}

	/** returns the overhead caused by a call path */
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

	/** XXX RN: this is horribly hacky */
	CgNodePtrSet getSuperNode(CgNodePtr conjunction) {

		CgNodePtrSet parentNodes = conjunction->getParentNodes();

		std::vector<CgNodePtrSet> parentsAncestors;
		for (auto parentNode : parentNodes) {
			parentsAncestors.push_back( getAncestors(parentNode) );
		}

		CgNodePtrSet superNode;

		for (CgNodePtrSet ancestors : parentsAncestors) {
			for (CgNodePtrSet otherAncestors : parentsAncestors) {

				if(ancestors==otherAncestors) {
					continue;
				}

				CgNodePtrSet intersection = set_intersect(ancestors, otherAncestors);
				superNode.insert(intersection.begin(), intersection.end());
			}
		}

		return superNode;
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
				if (ancestors.find(parentNode) == ancestors.end()) {
					workQueue.push(parentNode);
				}
			}
		}

		return ancestors;
	}

	CgNodePtrSet set_intersect(CgNodePtrSet a, CgNodePtrSet b) {

		CgNodePtrSet intersect;

		std::set_intersection(
				a.begin(),a.end(),
				b.begin(),b.end(),
				std::inserter(intersect, intersect.begin()));

		return intersect;
	}

}
