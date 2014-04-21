
#include <memory>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>

class CgNode {

public:
	CgNode(std::string function);
	void addCallsNode(std::shared_ptr<CgNode> functionWhichIsCalledNode);
	void addIsCalledByNode(std::shared_ptr<CgNode> functionByWhichItIsCalledNode);

	bool isSameFunction(std::shared_ptr<CgNode> cgNodeToCompareTo);

	std::string getFunctionName();

	std::vector<std::shared_ptr<CgNode> > getCallers();
	std::vector<std::shared_ptr<CgNode> > getCallees();

	void addNumberOfCalls(int calls);
	unsigned int getNumberOfCalls();

	void dumpToDot(std::ofstream& outputStream);

	void print();
	void printMinimal();


	void setFilename(std::string filename);
	void setLineNumber(int line);

private:
	std::vector<std::shared_ptr<CgNode> > calledNodes;
	std::vector<std::shared_ptr<CgNode> > isCalledByNodes;
	std::string functionName;
	unsigned int numberOfCalls;
	// for later use
	std::string filename;
	int line;
};
