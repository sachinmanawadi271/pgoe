#ifndef CG_NODE_H
#define CG_NODE_H


#include <memory>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>

#include <set>
#include <map>

#include <unordered_set>

enum CgNodeState {
	NONE,
	INSTRUMENT,
	UNWIND
};

class CgNode;
typedef std::shared_ptr<CgNode> 		CgNodePtr;	// hopefully this typedef helps readability

typedef std::set<CgNodePtr> 			CgNodePtrSet;
typedef std::unordered_set<CgNodePtr> 	CgNodePtrUnorderedSet;

class CgNode {

public:
	CgNode(std::string function);
	void addChildNode(CgNodePtr childNode);
	void addParentNode(CgNodePtr parentNode);
	void removeChildNode(CgNodePtr childNode);
	void removeParentNode(CgNodePtr parentNode);

	bool isSameFunction(CgNodePtr otherNode);

	std::string getFunctionName() const;

	const CgNodePtrSet& getChildNodes() const;
	const CgNodePtrSet& getParentNodes() const;

	void addCallData(CgNodePtr parentNode, unsigned long long calls, double timeInSeconds);
	unsigned long long getNumberOfCalls();
	unsigned long long getNumberOfCallsWithCurrentEdges();
	unsigned long long getNumberOfCalls(CgNodePtr parentNode);

	double getRuntimeInSeconds();
	void setRuntimeInSeconds(double newRuntimeInSeconds);
	unsigned long long getExpectedNumberOfSamples();


	void setState(CgNodeState state, int numberOfUnwindSteps = 0);
	bool isInstrumented();
	bool isUnwound();
	int getNumberOfUnwindSteps();

	// marker pos & dependent conjunction stuff
	CgNodePtrSet& getMarkerPositions();
	CgNodePtrSet& getDependentConjunctions();

	// spanning tree stuff
	void addSpantreeParent(CgNodePtr parentNode);
	bool isSpantreeParent(CgNodePtr);
	void reset();

	void updateNodeAttributes(int samplesPerSecond);

	bool hasUniqueCallPath();
	bool isLeafNode();
	bool isRootNode();

	bool hasUniqueParent();
	bool hasUniqueChild();
	CgNodePtr getUniqueParent();
	CgNodePtr getUniqueChild();

	void setFilename(std::string filename);
	void setLineNumber(int line);

	void dumpToDot(std::ofstream& outputStream);

	void print();
	void printMinimal();

	friend std::ostream& operator<< (std::ostream& stream, const CgNode& n);

private:
	std::string functionName;
	CgNodeState state;

	int numberOfUnwindSteps;
	unsigned long long numberOfCalls;

	// note that these metrics are based on a profile and might be pessimistic
	double runtimeInSeconds;
	unsigned long long expectedNumberOfSamples;

	CgNodePtrSet childNodes;
	CgNodePtrSet parentNodes;

	// parentNode -> number of calls by that parent
	std::map<CgNodePtr, unsigned long long> numberOfCallsBy;

	// if the node is a conjunction, these are the potentially instrumented nodes
	CgNodePtrSet potentialMarkerPositions;
	// if the node is a potential marker position, these conjunctions depend on its instrumentation
	CgNodePtrSet dependentConjunctions;

	// this is possibly the dumbest way to implement a spanning tree
	CgNodePtrSet spantreeParents;

	// node attributes
	bool uniqueCallPath;

	// for later use
	std::string filename;
	int line;
};

#endif
