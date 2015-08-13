#include "IPCGReader.h"

/** RN: note that the format is child -> parent for whatever reason.. */
CallgraphManager IPCGAnal::build(std::string filename) {

	CallgraphManager *cg = new CallgraphManager();

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
