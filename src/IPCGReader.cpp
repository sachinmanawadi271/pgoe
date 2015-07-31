#include "IPCGReader.h"

CallgraphManager IPCGAnal::build(std::string filename) {
  if(filename.empty()) {
    std::cerr << "Filename was empty" << std::endl;
    return false;
  }

  std::ifstream in;
  in.open(filename.c_str());

	CallgraphManager cg;

  std::string line;
  std::string parentFunction;
  while (in.good()) {
    std::getline(in, line);
    if (parentFunction.empty()){
      parentFunction = line;
		}

    if (line.front() == '-') {
			auto childFun(line.substr(2));
      cg.putEdge(parentFunction, childFun);
			CgNodePtr node = cg.findOrCreateNode(childFun);
			node->setLinesOfCode(loc);
    } else {
      parentFunction = line;

    }
  }
}
