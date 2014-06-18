
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

	std::set<std::shared_ptr<CgNode> > getCallees();
	std::set<std::shared_ptr<CgNode> > getCallers();

	void addNumberOfCalls(int calls, std::shared_ptr<CgNode> caller);
	unsigned int getNumberOfCalls();
	unsigned int getNumberOfCalls(std::shared_ptr<CgNode> parent);

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

private:
	std::string functionName;
	bool needsInstrumentation;

	std::set<std::shared_ptr<CgNode> > calledNodes;
	std::set<std::shared_ptr<CgNode> > isCalledByNodes;

	std::map<std::shared_ptr<CgNode>, unsigned int> numberOfCallsBy;

	// graph attributes
	bool uniqueParents;	//** XXX RN: main is not marked */
	bool leaf;

	// for later use
	std::string filename;
	int line;
};
