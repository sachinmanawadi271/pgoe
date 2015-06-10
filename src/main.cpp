#include <fstream>
#include <cstdlib>


#include "CubeReader.h"
#include "DotReader.h"

#include "Callgraph.h"

#include "SanityCheckEstimatorPhase.h"
#include "EdgeBasedOptimumEstimatorPhase.h"
#include "NodeBasedOptimumEstimatorPhase.h"
#include "ProximityMeasureEstimatorPhase.h"

void registerEstimatorPhases(CallgraphManager& cg) {
//	cg.registerEstimatorPhase(new InstrumentEstimatorPhase());
//	cg.registerEstimatorPhase(new RemoveUnrelatedNodesEstimatorPhase(true));
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

		cg.registerEstimatorPhase(new ProximityMeasureEstimatorPhase(std::string("/home/j_lehr/all_repos/aachen/ci841521_Instrumentor/common/codes/DROPS/scorep-20150605_1449_6396736960480334/profile.cubex")));
}

bool stringEndsWith(const std::string& s, const std::string& suffix) {
	return s.size() >= suffix.size()
			&& s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

int main(int argc, char** argv) {

	if (argc == 1) {
		std::cerr << "ERROR >> Usage: " << argv[0] << " /PATH/TO/CUBEX/PROFILE"
				<< " [TARGET_SAMPLES_PER_SECOND]" << std::endl;
		exit(-1);
	}

	int samplesPerSecond = 1000;
	if (argc > 2) {
		samplesPerSecond = atoi(argv[2]);
	}

	CallgraphManager cg;
	std::string filePath(argv[1]);

	if (stringEndsWith(filePath, ".cubex")) {
		cg = CubeCallgraphBuilder::build(filePath, samplesPerSecond);
	} else if (stringEndsWith(filePath, ".dot")) {
		cg = DOTCallgraphBuilder::build(filePath, samplesPerSecond);
	} else {
		std::cerr << "ERROR: Unknown file ending in " << filePath << std::endl;
		exit(-1);
	}

	registerEstimatorPhases(cg);

	cg.thatOneLargeMethod();

	std::cerr << "Before return" << std::endl;

	return EXIT_SUCCESS;
}
