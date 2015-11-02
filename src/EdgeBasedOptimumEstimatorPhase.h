#ifndef EDGEBASEDOPTIMUMESTIMATORPHASE_H_
#define EDGEBASEDOPTIMUMESTIMATORPHASE_H_

#include "EstimatorPhase.h"
#include "CgHelper.h"

#include <algorithm> // for std::set_intersection

struct CgEdge {
	unsigned long long calls;
	CgNodePtr child;
	CgNodePtr parent;

	bool operator<(const CgEdge& other) const {
		return std::tie(calls, child, parent)
				< std::tie(other.calls, other.child, other.parent);
	}

	friend bool operator==(const CgEdge& lhs, const CgEdge& rhs) {
		return std::tie(lhs.calls, lhs.child, lhs.parent)
						== std::tie(rhs.calls, rhs.child, rhs.parent);
	}

	friend std::ostream& operator<< (std::ostream& stream, const CgEdge& c) {
		stream << "(" << *(c.parent) << ", "<< *(c.child) << ", " << c.calls << ")";

		return stream;
 	}
};

typedef std::set<CgEdge> CgEdgeSet;

struct MoreCalls {
	bool operator() (const CgEdge& lhs, const CgEdge& rhs) {
		return lhs.calls < rhs.calls;
	}
};

/**
 * RN: this phase can run independent of all others except for RemoveUnrelatedNodes.
 * The result is the optimal edge based instrumentation.
 * Because of the edge based nature this phase is not compatible with the node based sanity check.
 */
class EdgeBasedOptimumEstimatorPhase : public EstimatorPhase {
public:
	EdgeBasedOptimumEstimatorPhase();
	~EdgeBasedOptimumEstimatorPhase();

	void modifyGraph(CgNodePtr mainMethod);

	void printReport();
protected:
	void printAdditionalReport();
private:
	int numberOfSkippedEdges;

	int errorsFound;
	void builtinSanityCheck();
	int checkParentsForOverlappingCallpaths(CgNodePtr conjunctionNode);
	CgEdgeSet getInstrumentationPathEdges(CgNodePtr startNode, CgNodePtr startsChild);

};

#endif
