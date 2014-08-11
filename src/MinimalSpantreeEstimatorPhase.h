
#ifndef MINIMALSPANTREEESTIMATORPHASE_H_
#define MINIMALSPANTREEESTIMATORPHASE_H_

#include "EstimatorPhase.h"
#include "CgHelper.h"

#include <algorithm> // for std::set_intersection

struct SpantreeEdge {
	unsigned long long calls;
	CgNodePtr child;
	CgNodePtr parent;

	bool operator<(const SpantreeEdge& other) const {
		return std::tie(calls, child, parent)
				< std::tie(other.calls, other.child, other.parent);
	}

	friend bool operator==(const SpantreeEdge& lhs, const SpantreeEdge& rhs) {
		return std::tie(lhs.calls, lhs.child, lhs.parent)
						== std::tie(rhs.calls, rhs.child, rhs.parent);
	}

	friend std::ostream& operator<< (std::ostream& stream, const SpantreeEdge& c) {
		stream << "(" << *(c.parent) << ", "<< *(c.child) << ", " << c.calls << ")";

		return stream;
 	}
};

typedef std::set<SpantreeEdge> SpantreeEdgeSet;

struct MoreCalls {
	bool operator() (const SpantreeEdge& lhs, const SpantreeEdge& rhs) {
		return lhs.calls < rhs.calls;
	}
};

/**
 * RN: this phase can run independent of all others except for RemoveUnrelatedNodes
 * The results turned out to be less optimal than we expected
 */
class MinimalSpantreeEstimatorPhase : public EstimatorPhase {
public:
	MinimalSpantreeEstimatorPhase();
	~MinimalSpantreeEstimatorPhase();

	void modifyGraph(CgNodePtr mainMethod);

	void printReport();
protected:
	void printAdditionalReport();
private:
	int numberOfSkippedEdges;

	int errorsFound;
	void builtinSanityCheck();
	int checkParentsForOverlappingCallpaths(CgNodePtr conjunctionNode);
	SpantreeEdgeSet getInstrumentationPathEdges(CgNodePtr startNode, CgNodePtr startsChild);

};

#endif
