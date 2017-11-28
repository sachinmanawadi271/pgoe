#ifndef CALLGRAPH_H
#define CALLGRAPH_H

#include "CgNode.h"
#include "CgHelper.h"

class Callgraph {
public:
	// Finds the main function in the CallGraph
	CgNodePtr findMain();
	CgNodePtr findNode(std::string functionName);

	void insert(CgNodePtr node);

	void eraseInstrumentedNode(CgNodePtr node);

	void erase(CgNodePtr node, bool rewireAfterDeletion=false, bool force=false);

	CgNodePtrSet::iterator begin();
	CgNodePtrSet::iterator end();
	CgNodePtrSet::const_iterator begin() const;
	CgNodePtrSet::const_iterator end() const;

	size_t size();
    CgNodePtrSet getGraph();
private:
	// this set represents the call graph during the actual computation
	CgNodePtrSet graph;
};

#endif
