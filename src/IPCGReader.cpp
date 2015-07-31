#include "IPCGReader.h"

CallgraphManager IPCGAnal::build(std::string filename) {
  if(filename.empty()) {
    std::cerr << "Filename was empty" << std::endl;
    return false;
  }

  std::ifstream in;
  in.open(filename.c_str());

	CallgraphManager *cg = new CallgraphManager();

  std::string line;
  std::string parentFunction;
	std::pair<std::string, int> funcNameAndLineCount;
	while (in.good()) {
    std::getline(in, line);
		if(line.empty()){
			continue;
		}
		std::cout << "Read line: " << line << std::endl;
    if (parentFunction.empty()){
      parentFunction = line.substr(line.find(' '));
			int lineCount = std::stoi(line.substr(line.rfind(' ')));
			funcNameAndLineCount = {parentFunction, lineCount};
		}

    if (line.front() == '-') {
			auto childFun(line.substr(2));
      cg->putEdge(parentFunction, childFun, 0, "", 0, .0);
    } else {
      parentFunction = line.substr(line.find(' '));
			int lineCount = std::stoi(line.substr(line.rfind(' ')));
			funcNameAndLineCount = {parentFunction, lineCount};
    }
		std::cout << "Read: " << funcNameAndLineCount.first << " " << funcNameAndLineCount.second << std::endl;
  }
	return *cg;
}
