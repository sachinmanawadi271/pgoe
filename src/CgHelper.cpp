#include "CgHelper.h"

int CgConfig::samplesPerSecond = 10000;

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

	/** returns overhead of all the instrumented nodes of the call path. (node based) */
	unsigned long long getInstrumentationOverheadOfPath(CgNodePtr node) {

		unsigned long long costInNanos = 0ULL;

		CgNodePtrSet instrumentationPaths = getInstrumentationPath(node);
		for (auto potentiallyMarked : instrumentationPaths) {

			// RN: the main function can be instrumented as this is part of the heuristic
			if (potentiallyMarked->isInstrumentedWitness()) {

				costInNanos += potentiallyMarked->getNumberOfCalls()
						* CgConfig::nanosPerInstrumentedCall;
			}
		}

		return costInNanos;
	}

	/** returns a pointer to the node that is instrumented up that call path */
	// TODO: check because of new nodeBased Conventions
	CgNodePtr getInstrumentedNodeOnPath(CgNodePtr node) {
		// XXX RN: this method has slowly grown up to a real mess
		if (node->isInstrumentedWitness() || node->isRootNode()) {
			return node;
		}

		if (isConjunction(node)) {
			return nullptr;
		}
		// single parent
		auto parentNode = getUniqueParent(node);


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

	/** Returns a set of all nodes from the starting node up to the instrumented nodes.
	 *  It should not break for cycles, because cycles have to be instrumented by definition. */
	CgNodePtrSet getInstrumentationPath(CgNodePtr start) {

		CgNodePtrSet path = { start };	// visited nodes
		std::queue<CgNodePtr> workQueue;
		workQueue.push(start);

		while (!workQueue.empty()) {

			auto node = workQueue.front();
			workQueue.pop();

			if (node->isInstrumented() || node->isRootNode()) {
				continue;
			}

			for (auto parentNode : node->getParentNodes()) {
				if (path.find(parentNode) != path.end()) {
					workQueue.push(parentNode);
					path.insert(parentNode);
				}
			}

		}

		return path;
	}

	bool isUniquelyInstrumented(CgNodePtr conjunctionNode) {

		CgNodePtrSet visited;	// visited nodes
		std::queue<CgNodePtr> workQueue;
		workQueue.push(conjunctionNode);

		while (!workQueue.empty()) {

			auto node = workQueue.front();
			workQueue.pop();

			if (visited.find(node) == visited.end()) {
				visited.insert(node);
			} else {
				return false;
			}

			if (node->isInstrumented() || node->isRootNode()) {
				continue;
			}

			for (auto parentNode : node->getParentNodes()) {
				workQueue.push(parentNode);
			}
		}
		return true;
	}

	/**
	 * Checks the instrumentation paths (node based!) above a conjunction node for intersection.
	 * Returns the Number Of Errors ! */
	int uniqueInstrumentationTest(CgNodePtr conjunctionNode) {

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

				auto intersection = setIntersect(pair.second, otherPair.second);

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

	/** returns true if the call paths of EVERY parent of a conjunction is instrumented */
	bool allParentsPathsInstrumented(CgNodePtr conjunctionNode) {
		auto parents = conjunctionNode->getParentNodes();

		return std::accumulate(parents.begin(), parents.end(), true,
				[] (bool b, CgNodePtr parent) {
					return b && (getInstrumentationOverheadOfPath(parent)!=0);
				});
	}

	/** returns the overhead caused by a call path */
	// TODO this will only work with direct parents instrumented
	unsigned long long getInstrumentationOverheadOfConjunction(
			CgNodePtr conjunctionNode) {

		auto parents = conjunctionNode->getParentNodes();

		CgNodePtrSet potentiallyInstrumented;
		for (auto parentNode : parents) {
			auto tmpSet = getInstrumentationPath(parentNode);
			potentiallyInstrumented.insert(tmpSet.begin(), tmpSet.end());
		}

		// add costs if node is instrumented
		return std::accumulate(
				potentiallyInstrumented.begin(), potentiallyInstrumented.end(), 0ULL,
				[] (unsigned long long acc, CgNodePtr node) {
					if (node->isInstrumentedWitness()) {
						return acc + (node->getNumberOfCalls()*CgConfig::nanosPerInstrumentedCall);
					}
					return acc;
				}
		);
	}

	/** returns the overhead caused by a call path */
	// TODO this will only work with direct parents instrumented
	unsigned long long getInstrumentationOverheadOfConjunction(
			CgNodePtrSet conjunctionNodes) {

		CgNodePtrSet allParents;
		for (auto n : conjunctionNodes) {
			CgNodePtrSet parents = n->getParentNodes();
			allParents.insert(parents.begin(), parents.end());
		}

		CgNodePtrSet potentiallyInstrumented;
		for (auto parentNode : allParents) {
			auto tmpSet = getInstrumentationPath(parentNode);
			potentiallyInstrumented.insert(tmpSet.begin(), tmpSet.end());
		}

		// add costs if node is instrumented
		return std::accumulate(
				potentiallyInstrumented.begin(), potentiallyInstrumented.end(), 0ULL,
				[] (unsigned long long acc, CgNodePtr node) {
					if (node->isInstrumentedWitness()) {
						return acc + (node->getNumberOfCalls()*CgConfig::nanosPerInstrumentedCall);
					}
					return acc;
				}
		);
	}

	unsigned long long getInstrumentationOverheadServingOnlyThisConjunction(
			CgNodePtr conjunctionNode) {

		auto parents = conjunctionNode->getParentNodes();

		CgNodePtrSet potentiallyInstrumented;
		for (auto parentNode : parents) {
			auto tmpSet = getInstrumentationPath(parentNode);
			potentiallyInstrumented.insert(tmpSet.begin(), tmpSet.end());
		}

		// add costs if node is instrumented
		return std::accumulate(
				potentiallyInstrumented.begin(), potentiallyInstrumented.end(), 0ULL,
				[] (unsigned long long acc, CgNodePtr node) {
					bool onlyOneDependendConjunction = node->getDependentConjunctions().size() == 1;
					if (node->isInstrumentedWitness() && onlyOneDependendConjunction) {
						return acc + (node->getNumberOfCalls()*CgConfig::nanosPerInstrumentedCall);
					}
					return acc;
				}
		);
	}

	unsigned long long getInstrumentationOverheadServingOnlyThisConjunction(
			CgNodePtrSet conjunctionNodes) {

		CgNodePtrSet allParents;
		for (auto n : conjunctionNodes) {
			CgNodePtrSet parents = n->getParentNodes();
			allParents.insert(parents.begin(), parents.end());
		}

		CgNodePtrSet potentiallyInstrumented;
		for (auto parentNode : allParents) {
			auto tmpSet = getInstrumentationPath(parentNode);
			potentiallyInstrumented.insert(tmpSet.begin(), tmpSet.end());
		}

		// add costs if node is instrumented
		return std::accumulate(
				potentiallyInstrumented.begin(), potentiallyInstrumented.end(), 0ULL,
				[] (unsigned long long acc, CgNodePtr node) {
					bool onlyOneDependendConjunction = node->getDependentConjunctions().size() == 1;
					if (node->isInstrumentedWitness() && onlyOneDependendConjunction) {
						return acc + (node->getNumberOfCalls()*CgConfig::nanosPerInstrumentedCall);
					}
					return acc;
				}
		);
	}


	/** removes the instrumentation of a call path.
	 * 	returns false if no instrumentation found */
	// TODO: check because of new nodeBased Conventions
	bool removeInstrumentationOnPath(CgNodePtr node) {
		if (node->isInstrumentedWitness()) {

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

	// Graph Stats
	CgNodePtrSet getPotentialMarkerPositions(CgNodePtr conjunction) {
		CgNodePtrSet potentialMarkerPositions;

		if (!CgHelper::isConjunction(conjunction)) {
			return potentialMarkerPositions;
		}

		CgNodePtrSet visitedNodes;
		std::queue<CgNodePtr> workQueue;
		workQueue.push(conjunction);

		while (!workQueue.empty()) {

			auto node = workQueue.front();
			workQueue.pop();

			for (auto& parentNode : node->getParentNodes()) {

				if (visitedNodes.find(parentNode) != visitedNodes.end()) {
					continue;
				} else {
					visitedNodes.insert(parentNode);
				}

				if (isValidMarkerPosition(parentNode, conjunction)) {
					potentialMarkerPositions.insert(parentNode);
					workQueue.push(parentNode);
				}
			}
		}

		///XXX
		assert(potentialMarkerPositions.size() >= (conjunction->getParentNodes().size()-1));

		return potentialMarkerPositions;
	}

	bool isValidMarkerPosition(CgNodePtr markerPosition, CgNodePtr conjunction) {

		if (isOnCycle(markerPosition)) {
			return true;	// nodes on cycles are always valid marker positions
		}

		// if one parent of the conjunction parents is unreachable -> valid marker
		for (auto parentNode : conjunction->getParentNodes()) {
			if (!reachableFrom(markerPosition, parentNode)) {
				return true;
			}
		}
		return false;
	}

	bool isOnCycle(CgNodePtr node) {
		CgNodePtrSet visitedNodes;
		std::queue<CgNodePtr> workQueue;
		workQueue.push(node);
		while (!workQueue.empty()) {
			auto currentNode = workQueue.front();
			workQueue.pop();

			if (visitedNodes.count(currentNode) == 0) {
				visitedNodes.insert(currentNode);

				for (auto child : currentNode->getChildNodes()) {

					if (child == node) {
						return true;
					}
					workQueue.push(child);
				}
			}
		}
		return false;
	}

	CgNodePtrSet getReachableConjunctions(CgNodePtrSet markerPositions) {
		CgNodePtrSet reachableConjunctions;

		CgNodePtrSet visitedNodes;
		std::queue<CgNodePtr> workQueue;
		for (auto markerPos : markerPositions) {
			workQueue.push(markerPos);
		}

		while (!workQueue.empty()) {
			auto node = workQueue.front();
			workQueue.pop();

			for (auto child : node->getChildNodes()) {
				if (visitedNodes.find(child) != visitedNodes.end()) {
					continue;
				} else {
					visitedNodes.insert(child);
				}

				workQueue.push(child);

				if (CgHelper::isConjunction(child)) {
					reachableConjunctions.insert(child);
				}
			}
		}
		return reachableConjunctions;
	}

	// note: a function is reachable from itself
	bool reachableFrom(CgNodePtr parentNode, CgNodePtr childNode) {

		if (parentNode == childNode) {
			return true;
		}

		// XXX RN: once again code duplication
		CgNodePtrSet visitedNodes;
		std::queue<CgNodePtr> workQueue;
		workQueue.push(parentNode);

		while (!workQueue.empty()) {

			auto node = workQueue.front();
			workQueue.pop();

			if (node == childNode) {
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
	// XXX RN: deprecated!
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

	/** Returns true if the unique call path property for edges is violated
	 *  once this edge is added */
	bool canReachSameConjunction(CgNodePtr below, CgNodePtr above) {

		CgNodePtrSet belowReachableDescendants = getDescendants(below);

		CgNodePtrSet aboveReachableDescendants;
		for (auto ancestor : getAncestors(above)) {
			CgNodePtrSet descendants = getDescendants(ancestor);
			aboveReachableDescendants.insert(descendants.begin(), descendants.end());
		}

		CgNodePtrSet intersect = setIntersect(belowReachableDescendants, aboveReachableDescendants);

		return !intersect.empty();
	}

	/** Returns a set of all descendants including the starting node */
	CgNodePtrSet getDescendants(CgNodePtr startingNode) {

		CgNodePtrSet childs;
		std::queue<CgNodePtr> workQueue;
		workQueue.push(startingNode);

		while (!workQueue.empty()) {

			auto node = workQueue.front();
			workQueue.pop();

			childs.insert(node);

			for (auto childNode : node->getChildNodes()) {
				if(childs.find(childNode) == childs.end()) {
					workQueue.push(childNode);
				}
			}
		}

		return childs;
	}

	/** Returns a set of all ancestors including the startingNode */
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

}
