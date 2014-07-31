
#ifndef OPTIMALNODEBASEDESTIMATORPHASE_H_
#define OPTIMALNODEBASEDESTIMATORPHASE_H_

#include "EstimatorPhase.h"
#include "CgNode.h"
#include "CgHelper.h"

#include <stack>
#include <vector>
#include <functional>	// std::hash

struct OptimalNodeBasedConstraint;
struct OptimalNodeBasedState;

typedef std::vector<OptimalNodeBasedConstraint> ConstraintContainer;


class OptimalNodeBasedEstimatorPhase : public EstimatorPhase {
public:
	OptimalNodeBasedEstimatorPhase();
	~OptimalNodeBasedEstimatorPhase();

	void modifyGraph(CgNodePtr mainMethod);

protected:
	void printAdditionalReport();

private:
	std::stack<OptimalNodeBasedState> stateStack;

	CgNodePtrSet optimalInstrumentation;
	unsigned long long optimalCosts;

	unsigned long long numberOfSteps;

	// optimization for large data sets
	std::set<std::size_t> visitedCombinations;

	void findStartingState(CgNodePtr mainMethod);

	void step();
};

struct OptimalNodeBasedConstraint {
	size_t size;
	CgNodePtrSet elements;

	CgNodePtr conjunction;

	CgNodePtrSet superNode;

	OptimalNodeBasedConstraint(CgNodePtrSet elements, CgNodePtr conjunction) {
		this->elements = elements;
		size = elements.size();

		this->conjunction = conjunction;

		this->superNode = CgHelper::getSuperNode(conjunction);
	}

	bool validAfterExchange(CgNodePtr oldElement, CgNodePtrSet newElements) {

		// if there is already an element part of superNode
		CgNodePtrSet elementsPartOfSuperNode = CgHelper::set_intersect(elements, superNode);

		///XXX
		std::cout << "# elementsPartOfSuperNode: " << elementsPartOfSuperNode.size() << std::endl;

		if (!elementsPartOfSuperNode.empty()) {

			bool oldIsPartOfSuperNode = (superNode.find(oldElement) != superNode.end());
			CgNodePtrSet newElementsPartOfSuperNode = CgHelper::set_intersect(elements, superNode);
			bool newIsPartOfSuperNode = !newElementsPartOfSuperNode.empty();

			if (!oldIsPartOfSuperNode && newIsPartOfSuperNode) {
				return false;
			}
		}

		if (elements.find(oldElement) != elements.end()) {
			size += (newElements.size() - 1);

			elements.erase(oldElement);
			elements.insert(newElements.begin(), newElements.end());
		}
		return elements.size() == size;
	}

	friend std::ostream& operator<< (std::ostream& stream, const OptimalNodeBasedConstraint& c) {
		stream << "(" << c.size << ")[";
		for (auto e : c.elements) {
			stream << *e << "|";
		}
		stream << "]";

		return stream;
	}

	bool operator==(const OptimalNodeBasedConstraint& other) const {
		return (size == other.size)
				&& (elements == other.elements);
	}

};

struct OptimalNodeBasedState {
	CgNodePtrSet nodeSet;
	ConstraintContainer constraints;

	OptimalNodeBasedState(CgNodePtrSet nodeSet, ConstraintContainer constraints) {
		this->nodeSet = nodeSet;
		this->constraints = constraints;
	}

	bool validAfterExchange(CgNodePtr oldElement, CgNodePtrSet newElements) {

		if(nodeSet.find(oldElement) != nodeSet.end()) {
			nodeSet.erase(oldElement);
			nodeSet.insert(newElements.begin(), newElements.end());
		}
		for (auto& constraint : constraints) {
			if (!constraint.validAfterExchange(oldElement, newElements)) {
				return false;
			}
		}

		return true;
	}

	unsigned long long getCosts() {
		// note that the scumbag zero will break everything unless it is explicitly "ULL"
		return std::accumulate( nodeSet.begin(), nodeSet.end(), 0ULL,
				[](unsigned long long calls, CgNodePtr node) {
					return calls + node->getNumberOfCalls();
				});
	}

	friend std::ostream& operator<< (std::ostream& stream, const OptimalNodeBasedState& state) {

		stream << "-- marked: ";
		for (auto node : state.nodeSet) {
			stream << *node << ", ";
		}
		stream << "-- constraints: ";
		for (auto c : state.constraints) {
			stream << c << ", ";
		}
		stream << "--";

		return stream;
	}
};

template <class T>
inline std::size_t hashCombine(const std::size_t seed, const T toBeHashed) {
	// according to stackoverflow this is a decent hash function
	return seed ^ ( std::hash<T>()(toBeHashed) + 0x9e3779b9 + (seed<<6) + (seed>>2) ) ;
}

namespace std {

	template <>
	struct hash<CgNodePtrSet> {
		inline size_t operator()(const CgNodePtrSet& key) const {

			return std::accumulate(
					key.begin(),
					key.end(),
					(size_t) 0,
					[](size_t acc, const CgNodePtr n) {
						// use pointer address for hash of CgNode
						return hashCombine<size_t>(acc, (size_t) n.get());
					}
			);
		}
	};

	template <>
	struct hash<ConstraintContainer> {
		size_t operator()(const ConstraintContainer& key) const {

			return std::accumulate(
					key.begin(),
					key.end(),
					(size_t) 0,
					[](size_t acc, const OptimalNodeBasedConstraint c) {
						return hashCombine<CgNodePtrSet>(acc, c.elements);
					}
			);
		}
	};

	template <>
	struct hash<OptimalNodeBasedState> {
		size_t operator()(const OptimalNodeBasedState& key) const {

			size_t hashFirst = hash<ConstraintContainer>()(key.constraints);
			return hashCombine<CgNodePtrSet>(hashFirst, key.nodeSet);
		}
	};
}


#endif
