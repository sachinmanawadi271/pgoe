#include <fstream>
#include <cstdlib>


#include "CubeReader.h"

#include "SanityCheckEstimatorPhase.h"
#include "EdgeBasedOptimumEstimatorPhase.h"
#include "NodeBasedOptimumEstimatorPhase.h"

//// POOR MAN'S CONFIG
#define PRINT_DOT 1

#define SAMPLES_PER_SECOND 1e3
////

void registerEstimatorPhases(Callgraph& cg) {
	cg.registerEstimatorPhase(new RemoveUnrelatedNodesEstimatorPhase());
	// edge based
	cg.registerEstimatorPhase(new EdgeBasedOptimumEstimatorPhase());

	cg.registerEstimatorPhase(new ResetEstimatorPhase());
	// heuristic
	cg.registerEstimatorPhase(new InstrumentEstimatorPhase());
	cg.registerEstimatorPhase(new MoveInstrumentationUpwardsEstimatorPhase());
	cg.registerEstimatorPhase(new DeleteOneInstrumentationEstimatorPhase());
	cg.registerEstimatorPhase(new UnwindEstimatorPhase());

	cg.registerEstimatorPhase(new SanityCheckEstimatorPhase());
	cg.registerEstimatorPhase(new ResetEstimatorPhase());
	// node based
//	cg.registerEstimatorPhase(new OptimalNodeBasedEstimatorPhase());
//	cg.registerEstimatorPhase(new SanityCheckEstimatorPhase());

}

int main(int argc, char** argv){

	if (argc == 1) {
		std::cerr << "ERROR >> Usage: " << argv[0] << " /PATH/TO/CUBEX/PROFILE"
				<< " [TARGET_SAMPLES_PER_SECOND]" << std::endl;
		exit(-1);
	}

	int samplesPerSecond = SAMPLES_PER_SECOND;
	if (argc > 2) {
		samplesPerSecond = atoi(argv[2]);
	}

	Callgraph cg = CubeCallgraphBuilder::build(argv[1], samplesPerSecond);

	registerEstimatorPhases(cg);

	cg.thatOneLargeMethod();


	return EXIT_SUCCESS;
}
