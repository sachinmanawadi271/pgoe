
#ifndef OPTIMALNODEBASEDESTIMATORPHASE_H_
#define OPTIMALNODEBASEDESTIMATORPHASE_H_

#include "EstimatorPhase.h"
#include "CgNode.h"
#include "CgHelper.h"

#include <stack>
#include <vector>

struct OptimalNodeBasedConstraint;
struct OptimalNodeBasedState;

typedef std::vector<std::shared_ptr<OptimalNodeBasedConstraint> > ConstraintContainer;


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

	// XXX RN: unused?
	bool validAfterExchange(CgNodePtr oldElement, CgNodePtr newElement) {
		elements.erase(oldElement);
		elements.insert(newElement);

		return elements.size() == size;
	}

	bool validAfterExchange(CgNodePtr oldElement, CgNodePtrSet newElements) {
		///XXX
		std::cout << (*this);

		size += (newElements.size() - 1);

		elements.erase(oldElement);
		elements.insert(newElements.begin(), newElements.end());

		///XXX
		std::cout << " --> " << (*this) << std::endl;

		return elements.size() == size;
	}

	friend std::ostream& operator<< (std::ostream& stream, const OptimalNodeBasedConstraint& c) {
		stream << "(" << c.size << ")[";
		for (auto e : c.elements) {
			stream << e->getFunctionName() << "|";
		}
		stream << "]";

		return stream;
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

		nodeSet.erase(oldElement);
		nodeSet.insert(newElements.begin(), newElements.end());

		for (auto constraint : constraints) {
			if (!constraint->validAfterExchange(oldElement, newElements)) {
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
			std::cout << node->getFunctionName() << ", ";
		}
		std::cout << "-- constraints: ";
		for (auto c : state.constraints) {
			std::cout << (*c) << ", ";
		}
		std::cout << "--";
			return stream;
	}
};


#endif
