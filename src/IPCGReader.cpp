#include "IPCGReader.h"

/** RN: note that the format is child -> parent for whatever reason.. */
CallgraphManager IPCGAnal::build(std::string filename) {

	CallgraphManager *cg = new CallgraphManager();

	std::ifstream file(filename);
	std::string line;

	std::string child;
	int childLoc = 0;

	while (std::getline(file, line)) {

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
				continue;
			}

			auto endPos = line.rfind(" ");
			child = line.substr(0,endPos);
			childLoc = std::stoi(line.substr(endPos));

			cg->putLinesOfCode(child, childLoc);
		}
	}
	return *cg;
}
