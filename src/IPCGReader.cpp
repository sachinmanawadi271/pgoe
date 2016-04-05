#include "IPCGReader.h"

/** RN: note that the format is child -> parent for whatever reason.. */
CallgraphManager IPCGAnal::build(std::string filename, Config* c) {

	CallgraphManager *cg = new CallgraphManager(c);

	std::ifstream file(filename);
	std::string line;

	std::string child;

	while (std::getline(file, line)) {

		if (line.empty()) {
			continue;
		}

		if (line.front() == '-') {
			// parent
			if (child.empty()) {
				continue;
			}
			std::string parent = line.substr(2);
			cg->putEdge(parent, std::string(), 0, child, 0, 0.0);
		} else {
			// child
			if (line.find("DUMMY")==0) {
				continue;	// for some reason the graph has these dummy edges
			}

			auto endPos = line.rfind(" ");	// space between name and numStmts
			child = line.substr(0,endPos);
			int childNumStmts=0;
			if (line.substr(line.rfind(' ')+1) == std::string("ND")){
				//CI: there was no defintion for the function or methods.
				//    ND = Not Definied
			}
			else {
				childNumStmts = std::stoi(line.substr(endPos));
				cg->putNumberOfStatements(child, childNumStmts);
			}
				

		}
	}

	cg->printDOT("reader");

	return *cg;
}

int IPCGAnal::addRuntimeDispatchCallsFromCubexProfile(CallgraphManager &ipcg, CallgraphManager &cubecg){
	// we iterate over all nodes of profile and check whether the IPCG based graph has the edge attached.
	// If not, we insert it.
	// This is pessimistic (safe for us) in the sense that, as a worst we instrument more than needed.
	int numNewlyInsertedEdges = 0;
	for(const auto cubeNode : cubecg){
		CgNodePtr ipcgEquivNode = ipcg.findOrCreateNode(cubeNode->getFunctionName());
		// Technically it can be, but we really want to know if so!
		assert(ipcgEquivNode != nullptr && "In the profile cannot be statically unknown nodes!");
		// now we want to compare the child nodes
		for(const auto callee : cubeNode->getChildNodes()){
			auto res = ipcgEquivNode->getChildNodes().find(callee); // XXX How is this comparison actually done?
			if(res == ipcgEquivNode->getChildNodes().end()){
				// we do not have the call stored!
				ipcg.putEdge(cubeNode->getFunctionName(), "ndef", 0, callee->getFunctionName(), callee->getNumberOfCalls(), .0);
				numNewlyInsertedEdges++;
			}
		}
	}
	return numNewlyInsertedEdges;
}
