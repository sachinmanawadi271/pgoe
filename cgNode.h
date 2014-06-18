
#include <memory>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>

// XXX RN: switch these out for unordered hash equivalents some time?
#include <map>
#include <set>


class CgNode {

public:
	CgNode(std::string function);
	void addChildNode(std::shared_ptr<CgNode> childNode);
	void addParentNode(std::shared_ptr<CgNode> parentNode);

	bool isSameFunction(std::shared_ptr<CgNode> otherNode);

	std::string getFunctionName();

	std::set<std::shared_ptr<CgNode> > getChildNodes();
	std::set<std::shared_ptr<CgNode> > getParentNodes();

	void addNumberOfCalls(int calls, std::shared_ptr<CgNode> parentNode);
	unsigned int getNumberOfCalls();
	unsigned int getNumberOfCalls(std::shared_ptr<CgNode> parentNode);

	void calcRelCallFrequency();
	void updateUniqueCallPathAttribute();
	bool hasUniqueCallPath();

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

	std::set<std::shared_ptr<CgNode> > childNodes;
	std::set<std::shared_ptr<CgNode> > parentNodes;

	std::map<std::shared_ptr<CgNode>, unsigned int> numberOfCallsBy;

	// graph attributes
	bool uniqueCallPath;	//** XXX RN: main is not marked */
	bool leafNode;

	// for later use
	std::string filename;
	int line;
};
