#include <fstream>
#include "callgraph.h"
#include "cube/Cube.h"
#include "cube/CubeMetric.h"
#include <cstdlib>

#include <cassert>

#define VERBOSE 5
#define PRINT_DOT 1

#define SAMPLES_PER_SECOND 1e7

int main(int argc, char** argv){


Callgraph cg(SAMPLES_PER_SECOND);
// The cube documentation says one should always use a try/catch thing around
try{
	// Create cube instance
	cube::Cube cube;
	// Read our cube file
	cube.openCubeReport( argv[1] );
	std::cout << "Reading input.. done." << std::endl;

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
	std::cout << "Finished construction of call graph ... numberOfCalls: " << overallNumberOfCalls
			<< " runtime: " << overallRuntime << " s" << std::endl;

	/** JP: This is code to estimate the generated overhead via call-graph guided hook placement. */
	const int overheadPerCallInNanos = 4; 
	unsigned long long overallOverhead = 0;
	unsigned long long numberOfInstrCalls = 0;
	unsigned long long optimizedOverhead = 0;
	unsigned long long optimizedNumberOfInstrCall = 0;

	cg.thatOneLargeMethod();

	// sum up the calls to function we would instrument
	for(auto node : cg.getNodesRequiringInstrumentation()){
		overallOverhead += overheadPerCallInNanos * node->getNumberOfCalls();
		numberOfInstrCalls += node->getNumberOfCalls();
	}

	// Now we can again sum up the calls to functions we would instrument...
	for(auto node : cg.getNodesRequiringInstrumentation()){
		optimizedOverhead += overheadPerCallInNanos * node->getNumberOfCalls();
		optimizedNumberOfInstrCall += node->getNumberOfCalls();
	}

#if PRINT_DOT
	cg.printDOT("mark");
#endif	
	std::cout << " ------ Statistics (DEPRECATED) ------ \nA cg-analysis instrumentation would mark: " << cg.getNodesRequiringInstrumentation().size() << " out of " << cg.getSize() << "\n" ;
	std::cout << "Function calls:\t\t\t" << overallNumberOfCalls << std::endl;
	std::cout << "# instr. Function Calls:\t" << numberOfInstrCalls << std::endl;
	std::cout << "OVH:\t\t\t\t" << (numberOfInstrCalls * overheadPerCallInNanos) / (1e9) << std::endl;
	std::cout << "Adding:\t\t\t\t" << overallOverhead << " nanos" << std::endl;
	std::cout << "Adding:\t\t\t\t" << (overallOverhead / (1e9)) << "seconds" << std::endl;
	std::cout << "After Optimization:\t\t" << (optimizedOverhead / (1e9)) << " seconds." << std::endl;
	std::cout << "Function calls optimized:\t" << optimizedNumberOfInstrCall << std::endl;
	std::cout << "Saving:\t\t\t\t" << ((overallOverhead - optimizedOverhead) / (1e9)) << std::endl;

} catch(const cube::RuntimeError& e){
	std::cout << e.get_msg() << std::endl;
}

return EXIT_SUCCESS;
}
