
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

typedef std::vector<CgNodePtrSet> CgNodePtrSetContainer;


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

	unsigned long long numberOfStepsTaken;
	unsigned long long numberOfStepsAvoided;

	// optimization for large data sets
	std::set<std::size_t> visitedCombinations;

	void findStartingState(CgNodePtr mainMethod);

	void step();
};

struct OptimalNodeBasedConstraint {
	size_t size;
	CgNodePtrSetContainer elements;

	CgNodePtr conjunction;	// maybe this will come handy later

	OptimalNodeBasedConstraint(CgNodePtrSetContainer elements, CgNodePtr conjunction) {
		this->elements = elements;
		this->size = elements.size();

		this->conjunction = conjunction;
	}

	bool validAfterExchange(CgNodePtr oldElement, CgNodePtrSet newElements) {

		for (CgNodePtrSet& element : elements) {
			if (element.find(oldElement) != element.end()) {

				element.insert(newElements.begin(), newElements.end());
			}
		}
		return isValid();
	}

	bool isValid() {
		for (CgNodePtrSet element : elements) {
			for (CgNodePtrSet otherElement : elements) {

				if (element == otherElement) {
					continue;
				}

				CgNodePtrSet intersection = CgHelper::set_intersect(element, otherElement);
				if (!intersection.empty()) {
					return false;
				}
			}
		}
		return true;
	}

	friend std::ostream& operator<< (std::ostream& stream, const OptimalNodeBasedConstraint& c) {
		stream << "(" << c.size << ")[";
		for (auto element : c.elements) {
			stream << "(";
			for (auto n : element) {
				stream << *n << "|";
			}
			stream << "),";
		}
		stream << "]";

		return stream;
	}

	// XXX RN: where was this needed?
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
	return seed ^ ( std::hash<T>()(toBeHashed) + 0x9e3779b9UL + (seed<<6) + (seed>>2) ) ;
}

namespace std {

	template <>
	struct hash<CgNodePtrSet> {
		inline size_t operator()(const CgNodePtrSet& key) const {

			size_t containerHash = std::hash<int>()((int) key.size());

			return std::accumulate(
					key.begin(),
					key.end(),
					(size_t) containerHash,
					[](size_t acc, const CgNodePtr n) {
						// use pointer address for hash of CgNode
						return hashCombine<size_t>(acc, (size_t) n.get());
					}
			);
		}
	};

	template <>
	struct hash<CgNodePtrSetContainer> {
		inline size_t operator()(const CgNodePtrSetContainer& key) const {

			size_t containerHash = std::hash<size_t>()(key.size());

			return std::accumulate(
					key.begin(),
					key.end(),
					(size_t) containerHash,
					[](size_t acc, const CgNodePtrSet ptrSet) {
						size_t innerHash = hash<CgNodePtrSet>()(ptrSet);
						return hashCombine<size_t>(acc, innerHash);
					}
			);
		}
	};

	template <>
	struct hash<ConstraintContainer> {
		size_t operator()(const ConstraintContainer& key) const {

			size_t containerHash = std::hash<size_t>()(key.size());

			return std::accumulate(
					key.begin(),
					key.end(),
					(size_t) containerHash,
					[](size_t acc, const OptimalNodeBasedConstraint c) {
						return hashCombine<CgNodePtrSetContainer>(acc, c.elements);
					}
			);
		}
	};

	template <>
	struct hash<OptimalNodeBasedState> {
		size_t operator()(const OptimalNodeBasedState& key) const {

			size_t nodeSetHash = hash<CgNodePtrSet>()(key.nodeSet);
			return hashCombine<ConstraintContainer>(nodeSetHash, key.constraints);
		}
	};
}

struct CalledMoreOften {
	bool operator() (const CgNodePtr& lhs, const CgNodePtr& rhs) {
		return lhs->getNumberOfCalls() < rhs->getNumberOfCalls();
	}
};


#endif
