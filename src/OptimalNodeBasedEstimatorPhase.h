
#ifndef OPTIMALNODEBASEDESTIMATORPHASE_H_
#define OPTIMALNODEBASEDESTIMATORPHASE_H_

#include "EstimatorPhase.h"
#include "CgNode.h"
#include "CgHelper.h"

#include <stack>
#include <vector>

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


	void findStartingState(CgNodePtr mainMethod);
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
		size += (newElements.size() - 1);

		elements.erase(oldElement);
		elements.insert(newElements.begin(), newElements.end());

		return elements.size() == size;
	}
};

struct OptimalNodeBasedState {
	CgNodePtrSet currentInstrumentation;
	ConstraintContainer constraints;

	OptimalNodeBasedState(CgNodePtrSet startingSet, ConstraintContainer startingConstraints) {
		this->currentInstrumentation = startingSet;
		this->constraints = startingConstraints;
	}

	bool validAfterExchange(CgNodePtr oldElement, CgNodePtrSet newElements) {

		currentInstrumentation.erase(oldElement);
		currentInstrumentation.insert(newElements.begin(), newElements.end());

		for (OptimalNodeBasedConstraint constraint : constraints) {
			if (!constraint.validAfterExchange(oldElement, newElements)) {
				return false;
			}
		}
		return true;
	}
};


#endif
