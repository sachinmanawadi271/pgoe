#include <fstream>
#include <cstdlib>

#include <cube/Cube.h>
#include <cube/CubeMetric.h>

#include "Callgraph.h"

#include "SanityCheckEstimatorPhase.h"
#include "EdgeBasedOptimumEstimatorPhase.h"
#include "NodeBasedOptimumEstimatorPhase.h"

//// POOR MAN'S CONFIG
#define PRINT_DOT 1

#define SAMPLES_PER_SECOND 1e3
////

void registerEstimatorPhases(Callgraph& cg) {
	cg.registerEstimatorPhase(new RemoveUnrelatedNodesEstimatorPhase());

	cg.registerEstimatorPhase(new EdgeBasedOptimumEstimatorPhase());

	cg.registerEstimatorPhase(new ResetEstimatorPhase());
	cg.registerEstimatorPhase(new InstrumentEstimatorPhase());
	cg.registerEstimatorPhase(new MoveInstrumentationUpwardsEstimatorPhase());
	cg.registerEstimatorPhase(new DeleteOneInstrumentationEstimatorPhase());
	cg.registerEstimatorPhase(new UnwindEstimatorPhase());
	cg.registerEstimatorPhase(new SanityCheckEstimatorPhase());

//	cg.registerEstimatorPhase(new ResetEstimatorPhase());
//	cg.registerEstimatorPhase(new OptimalNodeBasedEstimatorPhase());
//	cg.registerEstimatorPhase(new SanityCheckEstimatorPhase());

}

int main(int argc, char** argv){

if(argc==1) {
	std::cerr << "ERROR >> Usage: " << argv[0] << " /PATH/TO/CUBEX/PROFILE"
			<< " [TARGET_SAMPLES_PER_SECOND]" << std::endl;
	exit(-1);
}

int samplesPerSecond = SAMPLES_PER_SECOND;
if(argc>2) {
	samplesPerSecond = atoi(argv[2]);
}

Callgraph cg(samplesPerSecond);
registerEstimatorPhases(cg);

// The cube documentation says one should always use a try/catch thing around
try{
	// Create cube instance
	cube::Cube cube;
	// Read our cube file
	cube.openCubeReport( argv[1] );
	// Get the cube nodes
	const std::vector<cube::Cnode*>& cnodes = cube.get_cnodev();
	unsigned long long overallNumberOfCalls = 0;
	double overallRuntime = 0.0;

	cube::Metric* timeMetric = cube.get_met("time");
	cube::Metric* visitsMetric = cube.get_met("visits");

	const std::vector<cube::Thread*> threads = cube.get_thrdv();

	for(auto cnode : cnodes){
		// I don't know when this happens, but it does.
		if(cnode->get_parent() == NULL) {
			continue;
		}

		// Put the parent/child pair into our call graph
		auto parentNode = cnode->get_parent()->get_callee();	// RN: don't trust no one. It IS the parent node
		auto childnode = cnode->get_callee();

		for(unsigned int i = 0; i < threads.size(); i++) {
			unsigned long long numberOfCalls = (unsigned long long) cube.get_sev(visitsMetric, cnode, threads.at(i));
			double timeInSeconds = cube.get_sev(timeMetric, cnode, threads.at(i));

			cg.putFunction(parentNode->get_name(), parentNode->get_mod(), parentNode->get_begn_ln(),
					childnode->get_name(), numberOfCalls, timeInSeconds);

			overallNumberOfCalls += numberOfCalls;
			overallRuntime += timeInSeconds;
		}
	}
	std::cout << "Finished construction .."
			<< " numberOfCalls: " << overallNumberOfCalls
			<< " | runtime: " << overallRuntime << " s"
			<< " | samplesPerSecond: " << samplesPerSecond << std::endl;

	cg.thatOneLargeMethod();


#if PRINT_DOT
	cg.printDOT("final");
#endif	

} catch(const cube::RuntimeError& e){
	std::cout << e.get_msg() << std::endl;
}

return EXIT_SUCCESS;
}
