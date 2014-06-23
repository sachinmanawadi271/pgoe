#include <map>
#include <string>
#include <queue>

#include "cgNode.h"

class Callgraph {

public:
	Callgraph(int samplesPerSecond=10000);
	int putFunction(std::string parentName, std::string childName);
	int putFunction(std::string parentName, std::string parentFilename, int parentLine,
			std::string childName, unsigned long long numberOfCalls, double timeInSeconds);


	void print();
	void printDOT(std::string prefix);

	std::shared_ptr<CgNode> findNode(std::string functionName); // Finds FIRST node including functionName
	std::shared_ptr<CgNode> findMain();

	std::vector<std::shared_ptr<CgNode> > getNodesRequiringInstrumentation();
	int getSize();

	void updateNodeAttributes();

	int markNodesRequiringInstrumentation();
	int moveHooksUpwards();

private:
	std::map<std::string, std::shared_ptr<CgNode> > graph;

	const int samplesPerSecond;
};

// XXX RN: this abstract class was just a brief idea how to handle different estimators
// it will probably not be enough since we are looking for the the optimal solution (unwind vs. instrument)
class OverheadEstimator {
public:
	OverheadEstimator();
	virtual ~OverheadEstimator();

	virtual void findNodesToMark() = 0;
	virtual void estimateOverhead() = 0;

	std::set<std::shared_ptr<CgNode> > getMarkedNodes() {
		return markedNodes;
	}

	unsigned int getOverhead(std::shared_ptr<CgNode> node) {
		return overheadByNodes[node];
	}

protected:
	std::set< std::shared_ptr<CgNode> > markedNodes;
	std::map< std::shared_ptr<CgNode>, unsigned int > overheadByNodes;

	std::string name;
};
