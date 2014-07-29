
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

	OptimalNodeBasedConstraint(CgNodePtrSet elements) {
		this->elements = elements;
		size = elements.size();
	}

	bool validAfterExchange(CgNodePtr oldElement, CgNodePtrSet newElements) {

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

		return std::accumulate( nodeSet.begin(), nodeSet.end(), 0,
				[](unsigned long long calls, CgNodePtr node) {
					return calls += node->getNumberOfCalls();
				});
	}

	friend std::ostream& operator<< (std::ostream& stream, const OptimalNodeBasedState& state) {

		std::cout << "-- marked: ";
		for (auto node : state.nodeSet) {
			std::cout << *node << ", ";
		}
		std::cout << "-- constraints: ";
		for (auto c : state.constraints) {
			std::cout << c << ", ";
		}
		std::cout << "--";

		return stream;
	}
};


namespace std {
	template <>
	struct hash<CgNode> {
		size_t operator()(const CgNode& key) const {
			// TODO RN: use Pointer address instead?
			return hash<string>()(key.getFunctionName());
		}
	};

	template <>
	struct hash<CgNodePtrSet> {
		size_t operator()(const CgNodePtrSet& key) const {

			return std::accumulate(
					key.begin(),
					key.end(),
					0,
					[](size_t acc, const CgNodePtr n) {
						// TODO RN: safe hash function
						return acc ^ std::hash<CgNodePtr>()(n);
					}
			);
		}
	};

	template <>
	struct hash<OptimalNodeBasedConstraint> {
		size_t operator()(const OptimalNodeBasedConstraint& key) const {

			return hash<CgNodePtrSet>()(key.elements);
		}
	};

	template <>
	struct hash<ConstraintContainer> {
		size_t operator()(const ConstraintContainer& key) const {

			return std::accumulate(
					key.begin(),
					key.end(),
					0,
					[](size_t acc, const OptimalNodeBasedConstraint c) {
						// TODO RN: safe hash function
						return acc ^ std::hash<OptimalNodeBasedConstraint>()(c);
					}
			);
		}
	};

	template <>
	struct hash<OptimalNodeBasedState> {
		size_t operator()(const OptimalNodeBasedState& key) const {

			return hash<ConstraintContainer>()(key.constraints)
					^ hash<CgNodePtrSet>()(key.nodeSet);
		}
	};
}





#endif
