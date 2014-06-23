
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

	void addCallData(std::shared_ptr<CgNode> parentNode, unsigned long long calls, double timeInSeconds);
	unsigned long long getNumberOfCalls();
	unsigned long long getNumberOfCalls(std::shared_ptr<CgNode> parentNode);


	bool getNeedsUnwind();
	void setNeedsInstrumentation(bool needsInstrumentation);
	bool getNeedsInstrumentation();


	void updateNodeAttributes(int samplesPerSecond);

	bool hasUniqueCallPath();
	bool isLeafNode();

	double getRuntimeInSeconds();
	unsigned long long getExpectedNumberOfSamples();


	void setFilename(std::string filename);
	void setLineNumber(int line);

	void dumpToDot(std::ofstream& outputStream);

	void print();
	void printMinimal();

private:
	std::string functionName;
	bool needsInstrumentation;

	// note that these metrics are based on a profile and might be pessimistic
	double runtimeInSeconds;
	unsigned long long expectedNumberOfSamples;

	std::set<std::shared_ptr<CgNode> > childNodes;
	std::set<std::shared_ptr<CgNode> > parentNodes;

	std::map<std::shared_ptr<CgNode>, unsigned long long> numberOfCallsBy;

	// node attributes
	bool uniqueCallPath;
	bool leafNode;

	// for later use
	std::string filename;
	int line;
};
