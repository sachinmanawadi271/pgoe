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

void registerEstimatorPhases(CallgraphManager& cg, std::string otherPath) {
//	cg.registerEstimatorPhase(new RemoveUnrelatedNodesEstimatorPhase(true, false));

//	cg.registerEstimatorPhase(new WLCallpathDifferentiationEstimatorPhase());
	cg.registerEstimatorPhase(new ResetEstimatorPhase());
//
	cg.registerEstimatorPhase(new FirstNLevelsEstimatorPhase(1));
	cg.registerEstimatorPhase(new FirstNLevelsEstimatorPhase(2));
	cg.registerEstimatorPhase(new FirstNLevelsEstimatorPhase(3));
	cg.registerEstimatorPhase(new FirstNLevelsEstimatorPhase(4));
	cg.registerEstimatorPhase(new FirstNLevelsEstimatorPhase(5));
	cg.registerEstimatorPhase(new FirstNLevelsEstimatorPhase(6));
	cg.registerEstimatorPhase(new FirstNLevelsEstimatorPhase(7));
	cg.registerEstimatorPhase(new ResetEstimatorPhase());
//
	cg.registerEstimatorPhase(new InclStatementCountEstimatorPhase(10));
	cg.registerEstimatorPhase(new ResetEstimatorPhase());
	cg.registerEstimatorPhase(new InclStatementCountEstimatorPhase(50));
	cg.registerEstimatorPhase(new ResetEstimatorPhase());
	cg.registerEstimatorPhase(new InclStatementCountEstimatorPhase(100));
	cg.registerEstimatorPhase(new ResetEstimatorPhase());
	cg.registerEstimatorPhase(new InclStatementCountEstimatorPhase(150));
	cg.registerEstimatorPhase(new ResetEstimatorPhase());
	cg.registerEstimatorPhase(new InclStatementCountEstimatorPhase(200));

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

//		cg.registerEstimatorPhase(new ProximityMeasureEstimatorPhase(otherPath));
}

bool stringEndsWith(const std::string& s, const std::string& suffix) {
	return s.size() >= suffix.size()
			&& s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

int main(int argc, char** argv) {

	std::cout << "Usage: " << argv[0] << " /PATH/TO/CUBEX/PROFILE"
			<< " [-other /PATH/TO/PROFILE/TO/COMPARE/TO]"
			<< " [-samples NUMBER_OF_SAMPLES_PER_SECOND]"
			<< " [-ref UNINSTRUMENTED_RUNTIME_SECONDS]" << std::endl << std::endl;

	if (argc == 1) {
		std::cerr << "ERROR: too few arguments." << std::endl;
		exit(-1);
	}

	double uninstrumentedReferenceRuntime = .0;
	int samplesPerSecond = 1000;
	std::string otherPath;

	for(int i = 1; i < argc; ++i) {
		auto arg = std::string(argv[i]);

		if (arg=="-other") {
			otherPath = std::string(argv[++i]);
			continue;
		}
		if (arg=="-samples") {
			samplesPerSecond = atoi(argv[++i]);
			continue;
		}
		if (arg=="-ref") {
			uninstrumentedReferenceRuntime = atof(argv[++i]);
			continue;
		}
	}

	CallgraphManager cg;
	std::string filePath(argv[1]);

	if (stringEndsWith(filePath, ".cubex")) {
		cg = CubeCallgraphBuilder::build(filePath, samplesPerSecond, uninstrumentedReferenceRuntime);
	} else if (stringEndsWith(filePath, ".dot")) {
		cg = DOTCallgraphBuilder::build(filePath, samplesPerSecond);
	} else if (stringEndsWith(filePath, ".ipcg")){
		cg = IPCGAnal::build(filePath);
	}	else {
		std::cerr << "ERROR: Unknown file ending in " << filePath << std::endl;
		exit(-1);
	}

	registerEstimatorPhases(cg, otherPath);

	cg.thatOneLargeMethod();

	return EXIT_SUCCESS;
}
