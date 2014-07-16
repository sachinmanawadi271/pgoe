#ifndef CG_NODE_H
#define CG_NODE_H


#include <memory>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>

// XXX RN: switch these out for unordered hash equivalents some time?
#include <map>
#include <set>

enum CgNodeState {
	NONE,
	INSTRUMENT,
	UNWIND
};

class CgNode;
typedef std::shared_ptr<CgNode> CgNodePtr;	// hopefully this typedef helps readability

class CgNode {

public:
	CgNode(std::string function);
	void addChildNode(CgNodePtr childNode);
	void addParentNode(CgNodePtr parentNode);

	bool isSameFunction(CgNodePtr otherNode);

	std::string getFunctionName();

	std::set<CgNodePtr> getChildNodes();
	std::set<CgNodePtr> getParentNodes();

	void addCallData(CgNodePtr parentNode, unsigned long long calls, double timeInSeconds);
	unsigned long long getNumberOfCalls();
	unsigned long long getNumberOfCalls(CgNodePtr parentNode);

	double getRuntimeInSeconds();
	unsigned long long getExpectedNumberOfSamples();


	void setState(CgNodeState state);
	bool isUnwound();
	bool isInstrumented();

	// spanning tree stuff
	void addSpantreeParent(CgNodePtr parentNode);
	bool isSpantreeParent(CgNodePtr);

	void updateNodeAttributes(int samplesPerSecond);

	bool hasUniqueCallPath();
	bool isLeafNode();
	bool isRootNode();

	void setFilename(std::string filename);
	void setLineNumber(int line);

	void dumpToDot(std::ofstream& outputStream);

	void print();
	void printMinimal();

private:
	std::string functionName;
	CgNodeState state;

	// note that these metrics are based on a profile and might be pessimistic
	double runtimeInSeconds;
	unsigned long long expectedNumberOfSamples;

	std::set<CgNodePtr> childNodes;
	std::set<CgNodePtr> parentNodes;

	// parentNode -> number of calls by that parent
	std::map<CgNodePtr, unsigned long long> numberOfCallsBy;

	// this is possibly the dumbest way to implement a spanning tree
	std::set<CgNodePtr> spantreeParents;

	// node attributes
	bool uniqueCallPath;
	bool leafNode;

	// for later use
	std::string filename;
	int line;
};

#endif