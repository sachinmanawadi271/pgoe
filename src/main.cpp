#include <fstream>
#include <cstdlib>
#include <vector>

#include "CubeReader.h"
#include "DotReader.h"
#include "IPCGReader.h"

#include "Callgraph.h"

#include "SanityCheckEstimatorPhase.h"
#include "EdgeBasedOptimumEstimatorPhase.h"
#include "NodeBasedOptimumEstimatorPhase.h"
#include "ProximityMeasureEstimatorPhase.h"
#include "IPCGEstimatorPhase.h"

void registerEstimatorPhases(CallgraphManager& cg, std::vector<std::string> argv) {

	cg.registerEstimatorPhase(new FirstNLevelsEstimatorPhase(2));
	cg.registerEstimatorPhase(new FirstNLevelsEstimatorPhase(3));
	cg.registerEstimatorPhase(new FirstNLevelsEstimatorPhase(4));
	cg.registerEstimatorPhase(new FirstNLevelsEstimatorPhase(5));
	cg.registerEstimatorPhase(new FirstNLevelsEstimatorPhase(6));
	cg.registerEstimatorPhase(new FirstNLevelsEstimatorPhase(7));

	cg.registerEstimatorPhase(new ResetEstimatorPhase());

	cg.registerEstimatorPhase(new InclStatementCountEstimatorPhase(5));

//	cg.registerEstimatorPhase(new RemoveUnrelatedNodesEstimatorPhase(true));

//	cg.registerEstimatorPhase(new GraphStatsEstimatorPhase());
//	cg.registerEstimatorPhase(new DiamondPatternSolverEstimatorPhase());
	// edge based
//	cg.registerEstimatorPhase(new EdgeBasedOptimumEstimatorPhase());
//
//	cg.registerEstimatorPhase(new ResetEstimatorPhase());
//	// heuristic
//	cg.registerEstimatorPhase(new InstrumentEstimatorPhase());
//	cg.registerEstimatorPhase(new MoveInstrumentationUpwardsEstimatorPhase());
//	cg.registerEstimatorPhase(new DeleteOneInstrumentationEstimatorPhase());
//	cg.registerEstimatorPhase(new UnwindEstimatorPhase());
//
//	cg.registerEstimatorPhase(new SanityCheckEstimatorPhase());
//	cg.registerEstimatorPhase(new ResetEstimatorPhase());
	// node based
//	cg.registerEstimatorPhase(new OptimalNodeBasedEstimatorPhase());
//	cg.registerEstimatorPhase(new SanityCheckEstimatorPhase());

//		cg.registerEstimatorPhase(new ProximityMeasureEstimatorPhase(argv[2]));
}

bool stringEndsWith(const std::string& s, const std::string& suffix) {
	return s.size() >= suffix.size()
			&& s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

int main(int argc, char** argv) {

	if (argc == 1) {
		std::cerr << "ERROR >> Usage: " << argv[0] << " /PATH/TO/CUBEX/PROFILE"
				<< " PATH/TO/PROFILE/TO/COMPARE/TO" << std::endl;
		exit(-1);
	}

	int samplesPerSecond = 1000;
//	if (argc > 2) {
//		samplesPerSecond = atoi(argv[2]);
//	}

	std::vector<std::string> vecArgv(argc);
	for(int i = 0; i < argc; ++i){
		vecArgv[i] = std::string(argv[i]);
	}

	CallgraphManager cg;
	std::string filePath(vecArgv[1]);

	if (stringEndsWith(filePath, ".cubex")) {
		cg = CubeCallgraphBuilder::build(filePath, samplesPerSecond);
	} else if (stringEndsWith(filePath, ".dot")) {
		cg = DOTCallgraphBuilder::build(filePath, samplesPerSecond);
	} else if (stringEndsWith(filePath, "_ipcg")){
		cg = IPCGAnal::build(filePath);
	}	else {
		std::cerr << "ERROR: Unknown file ending in " << filePath << std::endl;
		exit(-1);
	}

	registerEstimatorPhases(cg, vecArgv);

	cg.thatOneLargeMethod();

	return EXIT_SUCCESS;
}
