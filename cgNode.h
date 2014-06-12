
#include <memory>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>

#include <map>

#include <set>		// for sanity checks
#include <cassert>	// for sanity checks

class CgNode {

public:
	CgNode(std::string function);
	void addCallsNode(std::shared_ptr<CgNode> functionWhichIsCalledNode);
	void addIsCalledByNode(std::shared_ptr<CgNode> functionByWhichItIsCalledNode);

	bool isSameFunction(std::shared_ptr<CgNode> cgNodeToCompareTo);

	std::string getFunctionName();

	std::vector<std::shared_ptr<CgNode> > getCallers();
	std::vector<std::shared_ptr<CgNode> > getCallees();

	void addNumberOfCalls(int calls, std::shared_ptr<CgNode> callee);
	unsigned int getNumberOfCalls();

	void calcRelCallFrequency();
	void updateUniqueParentsAttribute();
	bool hasUniqueParents();

	void setNeedsInstrumentation(bool needsInstrumentation);
	bool getNeedsInstrumentation();

	void setFilename(std::string filename);
	void setLineNumber(int line);


	void dumpToDot(std::ofstream& outputStream);

	void print();
	void printMinimal();

	// XXX RN: for internal use
	void sanityCheck() {
		typedef std::set<std::shared_ptr<CgNode> > CgNodeSet;
		auto uniqueCalledNodes = CgNodeSet(calledNodes.begin(), calledNodes.end());
		auto uniqueIsCalledByNodes = CgNodeSet(isCalledByNodes.begin(), isCalledByNodes.end());

		assert(calledNodes.size() == uniqueCalledNodes.size());
		assert(isCalledByNodes.size() == uniqueIsCalledByNodes.size());
	}

private:
	std::string functionName;
	bool needsInstrumentation;

	std::vector<std::shared_ptr<CgNode> > calledNodes;
	std::vector<std::shared_ptr<CgNode> > isCalledByNodes;

	std::map<std::shared_ptr<CgNode>, unsigned int> numberOfCallsBy;

	// graph attributes
	bool uniqueParents;
	bool leaf;

	// for later use
	std::string filename;
	int line;
};
